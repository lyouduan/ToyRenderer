#pragma once
#include "D3D12Resource.h"
class TD3D12Texture
{
public:
	TD3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }


public:
	TD3D12ResourceLocation ResourceLocation;


private:
	//std::vector<std::unique_ptr<TD3D12Sha>>
};

