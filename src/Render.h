#pragma once

#include <memory>
#include <vector>
#include "SceneCaptureCube.h"
#include "RenderTarget.h"
#include "D3D12DescriptorCache.h"
#include "ModelLoader.h"
#include "ShadowMap.h"
#include "CascadedShadowMap.h"

class SceneCaptureCube;

class TRender
{
public:
	TRender();
	~TRender();

	void Initalize();

	void SetDescriptorHeaps();

	void DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader, TD3D12ConstantBufferRef& passRef);
	void DrawMeshIBL(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader, TD3D12ConstantBufferRef& passRef);

	void CreateIBLEnvironmentMap();
	void CreateIBLIrradianceMap();
	void CreateIBLPrefilterMap();
	void CreateIBLLUT2D();

	void IBLRenderPass();

	// Deferred rendering
	void GbuffersPass();
	void TiledBaseLightCulling();
	void DeferredShadingPass();
	void GbuffersDebug();

	void SSAOPass();

	// ForwardPuls Rendering
	void PrePassDepthBuffer();
	void CullingLightPass();
	void ForwardPlusPass();
	void LightGridDebug();

	// Draw Light
	void LightPass();

	// shadow map
	void ShadowPass();
	void ShadowMapDebug();
	void ScenePass();

	// Compute VSM
	void GenerateVSM();
	void GenerateESM();
	void GenerateEVSM();
	void GenerateVSSM();
	// TODO
	void GenerateSAT();

	void CascadedShadowMapPass();


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

	bool GetEnableForwardPulsPass() { return bEnableForwardPuls; }
	void SetEnableForwardPulsPass(bool b) { bEnableForwardPuls = b; }

	bool GetbEnableShadowMap() { return bEnableShadowMap; }
	void SetbEnableShadowMap(bool b) { bEnableShadowMap = b; }

private:

	TD3D12ConstantBufferRef UpdatePassCbuffer();
	void UpdateSSAOPassCbuffer();
	
	void CreateSceneCaptureCube();

	void CreateGBuffersResource();

	void CreateGBuffersPSO();

	void CreateForwardPulsResource();

	void CreateShadowResource();

	void CreateCSMResource();

	std::vector<float> CalcGaussianWeights(float sigma);

private:

	// PBR and IBL
	bool bUseEquirectangularMap = false;
	bool bEnableIBLEnvLighting = true;
	bool bDebugGBuffers = false;
	bool bEnableForwardPuls = false;

	bool bEnableShadowMap = false;
	
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
	std::unique_ptr<D3D12DepthBuffer> GBufferDepth;
	TD3D12RWStructuredBufferRef TileLightInfoListRef;

	std::unique_ptr<D3D12ColorBuffer> TiledDepthDebugTexture;

	std::unique_ptr<D3D12ColorBuffer> SSAOTexture;
	std::unique_ptr<D3D12ColorBuffer> SSAOBlurTexture;

	TD3D12ConstantBufferRef SSAOCBRef;

	// forward plus
	std::unique_ptr<D3D12ColorBuffer> DebugMap;

	// ShadowMap
	uint32_t ShadowSize = 1024;
	std::unique_ptr<ShadowMap> m_ShadowMap;
	// VSM Shadow
	std::unique_ptr<D3D12ColorBuffer> m_VSMTexture;
	std::unique_ptr<D3D12ColorBuffer> m_VSMBlurTexture;

	TD3D12ConstantBufferRef GaussWeightsCBRef;

	// ESM Shadow
	std::unique_ptr<D3D12ColorBuffer> m_ESMTexture;
	// EVSM
	std::unique_ptr<D3D12ColorBuffer> m_EVSMTexture;

	// Sum Area Table
	std::unique_ptr<D3D12ColorBuffer> m_SATTexture;
	std::unique_ptr<D3D12ColorBuffer> m_SATMiddleTexture;

	// VSSM
	const static UINT VSSMMaxRadius = 5;
	std::vector<std::unique_ptr<D3D12ColorBuffer>> m_VSSMTextures;


	// Cascaded shadow map
	uint32_t CSMSize = 1024;
	std::unique_ptr<CascadedShadowMap> m_CascadedShadowMap;
};

 