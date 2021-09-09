=type compute

layout(rgba32f, binding = 1) uniform writeonly image2D outImage;

uniform sampler2D inImage;
uniform float sharpness;
uniform bool removeNoise;

=include common.shader

const float RCAS_LIMIT = 0.25 - 1.0 / 16.0;

vec4 RcasLoadFilter(ivec2 pos)
{
	return texelFetch(inImage, pos, 0);
}

void RcasInputFilter(inout vec3 color)
{
}

vec3 RcasFilter(ivec2 outPos)
{
	//   U
	// L C R
	//   D
	vec3 cU = RcasLoadFilter(outPos + ivec2(0, -1)).rgb;
	vec3 cL = RcasLoadFilter(outPos + ivec2(-1, 0)).rgb;
	vec3 cC = RcasLoadFilter(outPos).rgb;
	vec3 cR = RcasLoadFilter(outPos + ivec2(1, 0)).rgb;
	vec3 cD = RcasLoadFilter(outPos + ivec2(0, 1)).rgb;

	RcasInputFilter(cU);
	RcasInputFilter(cL);
	RcasInputFilter(cC);
	RcasInputFilter(cR);
	RcasInputFilter(cD);

	float lumU = lumMul2(cU);
	float lumL = lumMul2(cL);
	float lumC = lumMul2(cC);
	float lumR = lumMul2(cR);
	float lumD = lumMul2(cD);
	float maxLum = max5f(lumU, lumL, lumC, lumR, lumD);
	float minLum = min5f(lumU, lumL, lumC, lumR, lumD);

	float noise = abs(0.25 * (lumU + lumL + lumR + lumD) - lumC);
	noise = clamp(noise / (maxLum - minLum), 0.0, 1.0);
	noise = 1.0 - 0.5 * noise;

	vec3 min4 = min4Vec3(cU, cL, cR, cD);
	vec3 max4 = max4Vec3(cU, cL, cR, cD);

	vec3 hitMin = min4 / (max4 * 4.0);
	vec3 hitMax = (1.0 - max4) / (min4 * 4.0 - 4.0);

	vec3 vLobe = max(-hitMin, hitMax);
	float lobe = max(-RCAS_LIMIT, min(maxComponent(vLobe), 0.0)) * sharpness;
	if (removeNoise)
		lobe *= noise;

	return (cC + (cU + cL + cR + cD) * lobe) / (4.0 * lobe + 1.0);
}

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	vec3 res = RcasFilter(pos);
	imageStore(outImage, pos, vec4(res, 1.0));
}