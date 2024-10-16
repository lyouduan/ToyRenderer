#include "D3D12Resource.h"

TD3D12Resource::TD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState)
	: D3DResource(InD3DResource), CurrentState(InitState)
{
	if (D3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		GPUVirtualAddress = D3DResource->GetGPUVirtualAddress();
	}
	else
	{
		// GetGPUVirtualAddress() returns NULL for non-buffer resources.
	}
}

void TD3D12Resource::Map()
{
	// mapping for upload buffer
	ThrowIfFailed(D3DResource->Map(0, nullptr, &MappedBaseAddress));
}

TD3D12ResourceLocation::TD3D12ResourceLocation()
{
}

TD3D12ResourceLocation::~TD3D12ResourceLocation()
{
	ReleaseResource();
}

void TD3D12ResourceLocation::ReleaseResource()
{
	// 根据不同类型分配方式，释放资源
	switch (ResourceLocationType)
	{
	case TD3D12ResourceLocation::EResourceLocationType::StandAlone:
	{
		// 删除TD3D12Resource*指针
		delete UnderlyingResource;
		break;
	}
	case TD3D12ResourceLocation::EResourceLocationType::SubAlloction:
	{
		if (Allocator)
		{
			//
		}
		break;
	}

	default:
		break;
	}
}
