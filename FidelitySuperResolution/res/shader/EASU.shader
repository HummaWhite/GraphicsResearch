=type compute

layout(rgba32f, binding = 0) uniform writeonly image2D outImage;

uniform sampler2D inImage;
uniform vec4 con0;
uniform vec4 con1;
uniform vec4 con2;
uniform vec4 con3;

=include common.shader

vec4 gatherR(vec2 pos)
{
    return textureGather(inImage, pos, 0);
}

vec4 gatherG(vec2 pos)
{
    return textureGather(inImage, pos, 1);
}

vec4 gatherB(vec2 pos)
{
    return textureGather(inImage, pos, 2);
}

vec2 rotateVec2(vec2 v, vec2 rot)
{
	return vec2(dot(v, rot), -rot.y * v.x + rot.x * v.y);
}

void EasuTapFilter(
    inout vec3 accumColor,
    inout float accumWeight,
    vec2 offset,
    vec2 gradDir,	// (normalized)
    vec2 length,
    float negLobe,
    float clipDist,
    vec3 color)
{
    vec2 v = rotateVec2(offset, gradDir) * length;
    float d2 = min(dot(v, v), clipDist);

    float cBase = 25.0 / 16.0 * square(0.4 * d2 - 1.0) - 9.0 / 16.0;
    float cWindow = square(negLobe * d2 - 1.0);

    float weight = cBase * cWindow;

    accumColor += color * weight;
    accumWeight += weight;
}

void EasuSetFilter(
    inout vec2 gradDir,
    inout float length,
    vec2 pos,
    int whichPixel,
    float cU, float cL, float cC, float cR, float cD)
{
    // s t    -->    0 1
    // u v    -->    2 3
    float w;
    float xy = pos.x * pos.y;
    if (whichPixel == 0)
        w = (1.0 - pos.x) * (1.0 - pos.y);
    if (whichPixel == 1)
        w = pos.x * (1.0 - pos.y);
    if (whichPixel == 2)
        w = (1.0 - pos.x) * pos.y;
    if (whichPixel == 3)
        w = pos.x * pos.y;
    //   U
    // L C R
    //   D
    float dirX = cR - cL;
    gradDir.x += dirX * w;
    float lenX = max(abs(cR - cC), abs(cC - cL));
    lenX = clamp(abs(dirX) * approxInv(lenX), 0.0, 1.0);
    length += square(lenX) * w;

    float dirY = cD - cU;
    gradDir.y += dirY * w;
    float lenY = max(abs(cD - cC), abs(cC - cU));
    lenY = clamp(abs(dirY) * approxInv(lenY), 0.0, 1.0);
    length += square(lenY) * w;
}

vec3 EasuFilter(ivec2 outPos)
{
    vec2 pos = vec2(outPos) * con0.xy + con0.zw;
    vec2 flPos = floor(pos);
    pos -= flPos;
    //      +---+---+
    //      |   |   |
    //      +--(0)--+
    //      | b | c |
    //  +---F---+---+---+
    //  | e | f | g | h |
    //  +--(1)--+--(2)--+
    //  | i | j | k | l |
    //  +---+---+---+---+
    //      | n | o |
    //      +--(3)--+
    //      |   |   |
    //      +---+---+
    vec2 p0 = flPos * con1.xy + con1.zw;
    vec2 p1 = p0 + con2.xy;
    vec2 p2 = p0 + con2.zw;
    vec2 p3 = p0 + con3.zy;

    vec4 r0 = gatherR(p0), g0 = gatherG(p0), b0 = gatherB(p0);
    vec4 r1 = gatherR(p1), g1 = gatherG(p1), b1 = gatherB(p1);
    vec4 r2 = gatherR(p2), g2 = gatherG(p2), b2 = gatherB(p2);
    vec4 r3 = gatherR(p3), g3 = gatherG(p3), b3 = gatherB(p3);

    vec4 lum0 = b0 * 0.5 + (r0 * 0.5 + g0);
    vec4 lum1 = b1 * 0.5 + (r1 * 0.5 + g1);
    vec4 lum2 = b2 * 0.5 + (r2 * 0.5 + g2);
    vec4 lum3 = b3 * 0.5 + (r3 * 0.5 + g3);
    // a b
    // r g
    float lumB = lum0.r, lumC = lum0.g;
    float lumI = lum1.r, lumJ = lum1.g, lumF = lum1.b, lumE = lum1.a;
    float lumK = lum2.r, lumL = lum2.g, lumH = lum2.b, lumG = lum2.a;
    float lumO = lum3.b, lumN = lum3.a;

    vec2 dir = vec2(0.0);
    float len = 0.0;
    EasuSetFilter(dir, len, pos, 0, lumB, lumE, lumF, lumG, lumJ);
    EasuSetFilter(dir, len, pos, 1, lumC, lumF, lumG, lumH, lumK);
    EasuSetFilter(dir, len, pos, 2, lumF, lumI, lumJ, lumK, lumN);
    EasuSetFilter(dir, len, pos, 3, lumG, lumJ, lumK, lumL, lumO);
    len = square(len) * 0.25;

    float lenDirSq = dot(dir, dir);
    bool zero = lenDirSq < 1.0 / 32768.0;
    float lenDirInv = zero ? 1.0 : approxRsqrt(lenDirSq);
    dir.x = zero ? 1.0 : dir.x;
    dir *= lenDirInv;

    float stretch = approxInv(max(abs(dir.x), abs(dir.y)));
    vec2 len2 = vec2(1.0 + (stretch - 1.0) * len, 1.0 - 0.5 * len);
    float negLobe = 0.5 - 0.29 * len;
    float clipDist = approxInv(negLobe);
    //   b c
    // e f g h
    // i j k l
    //   n o
    vec3 min4 = min4Vec3(
        vec3(r1.z, g1.z, b1.z), vec3(r2.w, g2.w, b2.w), vec3(r1.y, g1.y, b1.z), vec3(r2.x, b2.x, g2.x));
    vec3 max4 = max4Vec3(
        vec3(r1.z, g1.z, b1.z), vec3(r2.w, g2.w, b2.w), vec3(r1.y, g1.y, b1.z), vec3(r2.x, b2.x, g2.x));

    vec3 color = vec3(0.0);
    float weight = 0.0;
    EasuTapFilter(color, weight, vec2(0.0, -1.0) - pos, dir, len2, negLobe, clipDist, vec3(r0.x, g0.x, b0.x));
    EasuTapFilter(color, weight, vec2(1.0, -1.0) - pos, dir, len2, negLobe, clipDist, vec3(r0.y, g0.y, b0.y));
    EasuTapFilter(color, weight, vec2(-1.0, 1.0) - pos, dir, len2, negLobe, clipDist, vec3(r1.x, g1.x, b1.x));
    EasuTapFilter(color, weight, vec2(0.0, 1.0) - pos, dir, len2, negLobe, clipDist, vec3(r1.y, g1.y, b1.y));
    EasuTapFilter(color, weight, vec2(0.0, 0.0) - pos, dir, len2, negLobe, clipDist, vec3(r1.z, g1.z, b1.z));
    EasuTapFilter(color, weight, vec2(-1.0, 0.0) - pos, dir, len2, negLobe, clipDist, vec3(r1.w, g1.w, b1.w));
    EasuTapFilter(color, weight, vec2(1.0, 1.0) - pos, dir, len2, negLobe, clipDist, vec3(r2.x, g2.x, b2.x));
    EasuTapFilter(color, weight, vec2(2.0, 1.0) - pos, dir, len2, negLobe, clipDist, vec3(r2.y, g2.y, b2.y));
    EasuTapFilter(color, weight, vec2(2.0, 0.0) - pos, dir, len2, negLobe, clipDist, vec3(r2.z, g2.z, b2.z));
    EasuTapFilter(color, weight, vec2(1.0, 0.0) - pos, dir, len2, negLobe, clipDist, vec3(r2.w, g2.w, b2.w));
    EasuTapFilter(color, weight, vec2(1.0, 2.0) - pos, dir, len2, negLobe, clipDist, vec3(r3.z, g3.z, b3.z));
    EasuTapFilter(color, weight, vec2(0.0, 2.0) - pos, dir, len2, negLobe, clipDist, vec3(r3.w, g3.w, b3.w));

    return min(max4, max(min4, color * approxInv(weight)));
}

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(outImage, pos, vec4(EasuFilter(pos), 1.0));
}