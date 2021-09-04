#include "../Common/Soft/Sampler/Samplers.h"
#include "../Common/Soft/Math/Transform.h"
#include "../Common/Soft/Buffer/Texture.h"

#include "../Common/Ext/Eigen/Core"
#include "../Common/Ext/Eigen/Dense"

typedef glm::vec3 Vec3f;
typedef glm::ivec3 Vec3i;
typedef glm::i8vec3 Vec3i8;

const float Phi = 0.618033988749894848f;
const float Length = glm::sqrt(1.0f + Phi * Phi);
const float Ks = 1.0f / Length;
const float Kt = Phi / Length;

const Vec3f DiceVertices[12] =
{
    { Ks, Kt, 0.0f }, { -Ks, Kt, 0.0f }, { Ks, -Kt, 0.0f }, { -Ks, -Kt, 0.0f },
    { 0.0f, Ks, Kt }, { 0.0f, -Ks, Kt }, { 0.0f, Ks, -Kt }, { 0.0f, -Ks, -Kt },
    { Kt, 0.0f, Ks }, { Kt, 0.0f, -Ks }, { -Kt, 0.0f, Ks }, { -Kt, 0.0f, -Ks }
};

std::array<Vec3f, 6> getDiceVerticesHemisphere(Vec3f W)
{
    Vec3f octSign = {
        W.x < 0.0f ? -1.0f : 1.0f,
        W.y < 0.0f ? -1.0f : 1.0f,
        W.z < 0.0f ? -1.0f : 1.0f };

    Vec3f va = Vec3f(Ks, Kt, 0.0f) * octSign;
    Vec3f vb = Vec3f(0.0f, Ks, Kt) * octSign;
    Vec3f vc = Vec3f(Kt, 0.0f, Ks) * octSign;

    Vec3f vaf = Vec3f(-Kt, 0.0f, Ks) * octSign;
    Vec3f vbf = Vec3f(Ks, -Kt, 0.0f) * octSign;
    Vec3f vcf = Vec3f(0.0f, Ks, -Kt) * octSign;

    return { va, vb, vc, vaf, vbf, vcf };
}

std::array<int8_t, 6> getDiceVertexIndices(Vec3f W)
{
    Vec3i8 octBit = { W.x < 0.0f, W.y < 0.0f, W.z < 0.0f };
    Vec3i8 octFlp = Vec3i8(1) - octBit;
    int8_t ia = 0 + octBit.y * 2 + octBit.x;
    int8_t ib = 4 + octBit.z * 2 + octBit.y;
    int8_t ic = 8 + octBit.z * 2 + octBit.x;

    int8_t iaf = 8 + octBit.z * 2 + octFlp.x;
    int8_t ibf = 0 + octFlp.y * 2 + octBit.x;
    int8_t icf = 4 + octFlp.z * 2 + octBit.y;
    return { ia, ib, ic, iaf, ibf, icf };
}

float evalLobe(Vec3f lobe, Vec3f W)
{
    float cosTheta = Math::satDot(lobe, W);
    if (cosTheta == 0.0f)
        return 0.0f;
    return 0.35f * Math::square(cosTheta) + 0.25f * Math::quad(cosTheta);
}

Vec3f evalRadiance(const std::array<Vec3f, 12> &coefs, Vec3f W)
{
    Vec3f res(0.0f);
    for (int i = 0; i < 12; i++)
        res += coefs[i] * evalLobe(DiceVertices[i], W);
    return res;
}

using ProjFunc = std::function<float(Vec3f)>;
template<int NSamples>
float convMonteCarlo(ProjFunc f, ProjFunc g)
{
    constexpr const float nSampleInv = 1.0f / NSamples;
    //IndependentSampler sampler;
    SimpleSobolSampler sampler(1, 1);

    float res = 0.0f;
    for (int i = 0; i < NSamples; i++)
    {
        Vec3f Wi = Transform::planeToSphere(sampler.get2D());
        res += f(Wi) * g(Wi) * nSampleInv * 0.25f * Math::PiInv;
        sampler.nextSample();
    }
    return res;
}

template<int NSamples>
std::array<Vec3f, 12> project(Texture &envMap)
{
    std::array<Vec3f, 12> coefs;
    Eigen::Matrix<float, 12, 12> gram[3];
    Eigen::Matrix<float, 12, 1> b[3];
    Eigen::Matrix<float, 12, 1> c[3];

    for (int i = 0; i < 12; i++)
    {
        for (int m = 0; m < 3; m++)
        {
            b[m](i, 0) = convMonteCarlo<NSamples>(
                [&](Vec3f W) {
                    Vec3f rad = envMap.getSpherical(W);
                    return Math::vecElement(rad, m);
                },
                [&](Vec3f W) {
                    return evalLobe(DiceVertices[i], W);
                }
            );
        }
    }
    for (int i = 0; i < 12; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            for (int m = 0; m < 3; m++)
            {
                gram[m](i, j) = convMonteCarlo<NSamples>(
                    [&](Vec3f W) {
                        return evalLobe(DiceVertices[i], W);
                    },
                    [&](Vec3f W) {
                        return evalLobe(DiceVertices[j], W);
                    }
                );
            }
        }
    }
    for (int m = 0; m < 3; m++)
    {
        c[m] = gram[m].inverse() * b[m];
        for (int i = 0; i < 12; i++)
            Math::vecElement(coefs[i], m) = c[m](i, 0);
    }
    return coefs;
};

int main(int argc, char *argv[])
{
    if (argc < 3)
        return 0;

    const std::string name(argv[2]);
    const std::string op(argv[1]);

    Texture envMap;
    envMap.loadFloat((name + ".hdr").c_str());

    if (op == "-t")
    {
        envMap.saveImage(name + "_ori.png");
        return 0;
    }

    auto coefs = project<65536>(envMap);
    for (auto c : coefs)
        Math::printVec3(c);
    
    Texture outImage;
    outImage.init(400, 200);
    outImage.fill(Vec3f(0.0f));

    for (int b = 0; b < 12; b++)
    {
        for (int i = 0; i < outImage.width; i++)
        {
            for (int j = 0; j < outImage.height; j++)
            {
                float u = static_cast<float>(i) / outImage.width;
                float v = static_cast<float>(j) / outImage.height;
                Vec3f W = Transform::planeToSphere({u, v});
                //outImage(i, j) = evalRadiance(coefs, W);
                outImage(i, j) = coefs[b] * evalLobe(DiceVertices[b], W);
            }
        }
        outImage.saveImage(name + "_c" + std::to_string(b) + ".png");
    }
    return 0;
}