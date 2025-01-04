#pragma once
#include <stdint.h>
#include <DirectXMath.h>
#include <d3d12.h> 

using namespace DirectX;
// Aligns a value to the nearest higher multiple of 'Alignment'.
inline uint32_t AlignArbitrary(uint32_t Val, uint32_t Alignment)
{
	return ((Val + Alignment - 1) / Alignment) * Alignment;
}

template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> __forceinline T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

namespace MATH
{
    constexpr XMFLOAT4X4 IdentityMatrix = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
    };
}


// hlsl data struct
__declspec(align(16))
struct ObjCBuffer
{
    XMFLOAT4X4 ModelMat = MATH::IdentityMatrix;
};

__declspec(align(16))
struct PassCBuffer
{
    XMFLOAT4X4 ViewMat = MATH::IdentityMatrix;
    XMFLOAT4X4 ProjMat = MATH::IdentityMatrix;
    XMFLOAT3 EyePosition = {0.0, 0.0 ,0.0};
    FLOAT Pad0 = 0.0;
};
