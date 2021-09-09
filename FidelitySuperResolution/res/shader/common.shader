=type lib

float square(float x)
{
    return x * x;
}

float approxInv(float x)
{
    return 1.0 / x;
}

float approxRsqrt(float x)
{
    return 1.0 / sqrt(x);
}

vec3 min4Vec3(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return min(min(min(a, b), c), d);
}

vec3 max4Vec3(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return max(max(max(a, b), c), d);
}

float min5f(float a, float b, float c, float d, float e)
{
    return min(min(min(min(a, b), c), d), e);
}

float max5f(float a, float b, float c, float d, float e)
{
    return max(max(max(max(a, b), c), d), e);
}

float maxComponent(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

float minComponent(vec3 v)
{
    return min(min(v.x, v.y), v.z);
}

float lumMul2(vec3 color)
{
    return dot(color, vec3(0.5, 1.0, 0.5));
}