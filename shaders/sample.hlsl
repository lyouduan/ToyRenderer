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
#endif // !MATH_SAMPLE_H