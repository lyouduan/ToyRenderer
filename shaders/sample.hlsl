#ifndef MATH_SAMPLE_H
#define MATH_SAMPLE_H

#define PI 3.14159265359

float4 Quat_Inverse(float4 q)
{
    return float4(-q.xyz, q.w);
}

float3 Quat_Rotate(float4 q, float3 p)
{
	// Quat_Mul(Quat_Mul(q, vec4(p, 0)), Quat_Inverse(q)).xyz;

    float4 qp = float4(q.w * p + cross(q.xyz, p), -dot(q.xyz, p));
    float4 invQ = Quat_Inverse(q);
    float3 qpInvQ = qp.w * invQ.xyz + invQ.w * qp.xyz + cross(qp.xyz, invQ.xyz);
    return qpInvQ;
}
float4 Quat_ZTo(float3 to)
{
	// from = (0, 0, 1)
	//float cosTheta = dot(from, to);
	//float cosHalfTheta = sqrt(max(0, (cosTheta + 1) * 0.5));
    float cosHalfTheta = sqrt(max(0, (to.z + 1) * 0.5));
	//vec3 axisSinTheta = cross(from, to);
	//    0    0    1
	// to.x to.y to.z
	//vec3 axisSinTheta = vec3(-to.y, to.x, 0);
    float twoCosHalfTheta = 2 * cosHalfTheta;
    return float4(-to.y / twoCosHalfTheta, to.x / twoCosHalfTheta, 0, cosHalfTheta);
}

float2 UniformOnDisk(float Xi)
{
    float theta = 2.0 * PI * Xi;
    return float2(cos(theta), sin(theta));
}

float3 CosOnHalfSphere(float2 Xi)
{
    float r = sqrt(Xi.x);
    float2 pInDisk = r * UniformOnDisk(Xi.y);
    float z = sqrt(1 - Xi.x);
    return float3(pInDisk, z);
}


float3 CosOnHalfSphere(float2 Xi, float3 N)
{
    float3 p = CosOnHalfSphere(Xi);
    float4 rot = Quat_ZTo(N);
    return Quat_Rotate(rot, p);
}


float _RadicalInverse_VdC(uint bits)
{
	// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	// efficient VanDerCorpus calculation.

    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), _RadicalInverse_VdC(i));
}

float4 ImportanceSampleGGX(float2 Xi, float3 N, float Roughness)
{
    float a = Roughness * Roughness;
    float a2 = a * a;

    float Phi = 2.0 * PI * Xi.x;
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
    float SinTheta = sqrt(1.0 - CosTheta * CosTheta);

	// From spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(Phi) * SinTheta;
    H.y = sin(Phi) * SinTheta;
    H.z = CosTheta;

	// From tangent-space vector to world-space sample vector
    float3 Up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 TangentX = normalize(cross(Up, N));
    float3 TangentY = cross(N, TangentX);

    float3 SampleVec = TangentX * H.x + TangentY * H.y + N * H.z;
    SampleVec = normalize(SampleVec);
	
	// Calculate PDF
    float d = (CosTheta * a2 - CosTheta) * CosTheta + 1;
    float D = a2 / (PI * d * d);
    float PDF = D * CosTheta;
	
    return float4(SampleVec, PDF);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float3 UniformOnSphere(float2 Xi)
{
    float t = 2 * sqrt(Xi.x * (1 - Xi.x));
    float phi = 2 * PI * Xi.y;

    float x = cos(phi) * t;
    float y = sin(phi) * t;
    float z = 1 - 2 * Xi.x;

    return float3(x, y, z);
}

static const float2 poissonDisk[16] =
{ 
    float2(-0.942, -0.399), float2(0.945, -0.768),
    float2(-0.094, -0.929), float2(0.344, 0.293),
    float2(-0.915, 0.457), float2(-0.815, -0.879),
    float2(-0.382, 0.276), float2(0.974, 0.756),
    float2(0.443, -0.975), float2(0.537, -0.473),
    float2(-0.264, -0.418), float2(0.791, 0.190),
    float2(-0.241, 0.997), float2(-0.814, 0.914),
    float2(0.199, 0.786), float2(0.143, 0.141)
};

float2 PoissonDisk(uint i)
{
    return poissonDisk[i % 16];
}
//------------------------------------------------------------------------------
// Random
//------------------------------------------------------------------------------

/*
 * Random number between 0 and 1, using interleaved gradient noise.
 * w must not be normalized (e.g. window coordinates)
 */

float nrand(in float2 uv)
{
    float2 noise =
      (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}
// rotate the poisson disk randomly
float2x2 getRandomRotationMatrix(float2 uv) {
    // 条状阴影
    //float randomAngle = interleavedGradientNoise(uv) * 2.0 * PI;
    float randomAngle = nrand(uv) * 2.0 * PI;
    float sinAngle, cosAngle;
    sincos(randomAngle, sinAngle, cosAngle);
    float2x2 R = float2x2(cosAngle, sinAngle, -sinAngle, cosAngle);
    return R;
}

static const float2 BlueNoiseSamples[16] =
{
    float2(-0.6, -0.2), float2(0.8, -0.5), float2(-0.3, -0.9),
    float2(0.6, 0.3), float2(-0.9, 0.7), float2(-0.4, -0.8),
    float2(-0.1, 0.4), float2(0.7, 0.2), float2(0.3, -0.9),
    float2(0.2, -0.4), float2(-0.7, 0.6), float2(-0.9, 0.2),
    float2(0.4, 0.9), float2(-0.6, -0.5), float2(0.1, 0.3),
    float2(0.5, -0.7)
};

float2 GetBlueNoiseSample(int i)
{
    return BlueNoiseSamples[i % 16];
}

#endif // !MATH_SAMPLE_H