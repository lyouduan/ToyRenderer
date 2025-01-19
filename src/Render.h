#pragma once

#include <memory>
#include <vector>
#include "SceneCaptureCube.h"
#include "RenderTarget.h"
#include "D3D12DescriptorCache.h"
#include "ModelLoader.h"

class SceneCaptureCube;

class TRender
{
public:
	TRender();
	~TRender();

	void Initalize();
	void DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader, TD3D12ConstantBufferRef& passRef);

	void CreateIBLEnvironmentMap();
	void CreateIBLIrradianceMap();
	void CreateIBLPrefilterMap();
	void CreateIBLLUT2D();

	// Deferred rendering
	void GbuffersPass();
	void DeferredShadingPass();
	void GbuffersDebugPass();

	std::unique_ptr<SceneCaptureCube>& GetIBLEnvironmemtMap() { return IBLEnvironmentMap; }
	std::unique_ptr<SceneCaptureCube>& GetIBLIrradianceMap() { return IBLIrradianceMap; }
	std::unique_ptr<SceneCaptureCube>& GetIBLPrefilterMap(int i) 
	{ 
		assert(i >= 0 && i < IBLPrefilterMaxMipLevel);

		return IBLPrefilterMaps[i]; 
	}
	std::vector<std::unique_ptr<SceneCaptureCube>>& GetIBLPrefilterMaps() { return IBLPrefilterMaps; }

	std::unique_ptr<RenderTarget2D>& GetIBLBrdfLUT2D() { return IBLBrdfLUT2D; }

	bool GetEnableIBLEnvLighting() { return bEnableIBLEnvLighting; }
	void SetEnableIBLEnvLighting(bool b) { bEnableIBLEnvLighting = b; }

	bool GetbUseEquirectangularMap() { return bUseEquirectangularMap; }
	void SetbUseEquirectangularMap(bool b) { bUseEquirectangularMap = b; }

	bool GetbDebugGBuffers() { return bDebugGBuffers; }
	void SetbDebugGBuffers(bool b) { bDebugGBuffers = b; }

	std::unique_ptr<RenderTarget2D>& GetGBufferAlbedo() { return GBufferAlbedo; }
	std::unique_ptr<RenderTarget2D>& GetGBufferSpecular() { return  GBufferSpecular; }
	std::unique_ptr<RenderTarget2D>& GetGBufferWorldPos() { return GBufferWorldPos; }
	std::unique_ptr<RenderTarget2D>& GetGBufferNormal() { return GBufferNormal; }

	bool GetEnableDeferredRendering() { return bEnableDeferredRendering; }
	void SetEnableDeferredRendering(bool b) { bEnableDeferredRendering = b; }
private:

	void CreateSceneCaptureCube();

	void CreateGBuffers();

private:

	// PBR and IBL
	bool bUseEquirectangularMap = false;
	bool bEnableIBLEnvLighting = false;
	bool bDebugGBuffers = false;
	
	const static UINT IBLPrefilterMaxMipLevel = 5;
	std::unique_ptr<SceneCaptureCube> IBLEnvironmentMap;
	std::unique_ptr<SceneCaptureCube> IBLIrradianceMap;
	std::vector<std::unique_ptr<SceneCaptureCube>> IBLPrefilterMaps;
	std::unique_ptr<RenderTarget2D> IBLBrdfLUT2D;

	// Deferred Redenring
	// GBuffer info
	bool bEnableDeferredRendering = false;
	std::unique_ptr<RenderTarget2D> GBufferAlbedo;
	std::unique_ptr<RenderTarget2D> GBufferSpecular;
	std::unique_ptr<RenderTarget2D> GBufferWorldPos;
	std::unique_ptr<RenderTarget2D> GBufferNormal;

	std::unique_ptr<TD3D12DescriptorCache> GBufferDescriptorCache;
};

 