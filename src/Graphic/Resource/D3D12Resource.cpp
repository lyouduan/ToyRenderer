#include "D3D12Resource.h"
#include "DXSamplerHelper.h"

TD3D12Resource::TD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState)
	: D3DResource(InD3DResource), CurrentState(InitState)
{
	// Resource is buffer
	if (InD3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		GPUVirtualAddress = InD3DResource->GetGPUVirtualAddress();
	}
	else
	{
		// return NULL if non-buffer resource
	}
}

TD3D12Resource::TD3D12Resource(ID3D12Resource* InD3DResource, D3D12_RESOURCE_STATES InitState)
	: D3DResource(InD3DResource), CurrentState(InitState)
{
	// Resource is buffer
	if (InD3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		GPUVirtualAddress = InD3DResource->GetGPUVirtualAddress();
	}
	else
	{
		// return NULL if non-buffer resource
	}
}

void TD3D12Resource::Map()
{
	ThrowIfFailed(D3DResource->Map(0, nullptr, &MappedBaseAddress));
}

void TD3D12Resource::Destroy()
{
	D3DResource = nullptr;
	GPUVirtualAddress = 0;
	MappedBaseAddress = nullptr;
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
	switch (ResourceLocationType)
	{
	case TD3D12ResourceLocation::EResourceLocationType::StandAlone:
	{
		delete UnderlyingResource;
		break;
	}
	case TD3D12ResourceLocation::EResourceLocationType::SubAllocation:
	{
		if (Allocator)
		{
			//Allocator->Dellocate(*this);
		}
		break;
	}
	default:
		break;
	}
}
