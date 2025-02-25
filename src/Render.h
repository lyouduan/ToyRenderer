#pragma once

#include <memory>
#include <vector>
#include "SceneCaptureCube.h"
#include "RenderTarget.h"
#include "D3D12DescriptorCache.h"
#include "ModelLoader.h"
#include "ShadowMap.h"
#include "CascadedShadowMap.h"

#define TAA_SAMPLE_COUNT 8

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
	void HBAOPass();
	void TAAPass();

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

	// CSM
	void CascadedShadowMapPass();

	// post processing
	void FXAAPass();

	void EndFrame();

public:

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

	bool GetbEnableTAA() { return bEnableTAA; }
	void SetbEnableTAA(bool b) { bEnableTAA = b; }

	bool GetbEnableFXAA() { return bEnableFXAA; }
	void SetbEnableFXAA(bool b) { bEnableFXAA = b; }

private:

	TD3D12ConstantBufferRef UpdatePassCbuffer();

	void UpdateSSAOPassCbuffer();

	void CreateInputLayouts();

	void CreateSceneCaptureCube();

	void CreateGBuffersResource();

	void CreateAOResource();

	void CreateGBuffersPSO();

	void CreateTAAResoure();

	void CreateForwardPulsResource();

	void CreateShadowResource();

	void CreateCSMResource();

	void CreateFXAAResource();

	std::vector<float> CalcGaussianWeights(float sigma);

private:
	UINT m_RenderFrameCount = 0;

	std::vector<D3D12_INPUT_ELEMENT_DESC>  DefaultInputLayout;

	// PBR and IBL
	bool bUseEquirectangularMap = false;
	bool bEnableIBLEnvLighting = true;
	bool bEnableDeferredRendering = false;
	bool bDebugGBuffers = false;
	bool bEnableForwardPuls = false;
	bool bEnableTAA = false;
	bool bEnableFXAA = false;

	bool bEnableShadowMap = false;
	
	const static UINT IBLPrefilterMaxMipLevel = 5;
	std::unique_ptr<SceneCaptureCube> IBLEnvironmentMap;
	std::unique_ptr<SceneCaptureCube> IBLIrradianceMap;
	std::vector<std::unique_ptr<SceneCaptureCube>> IBLPrefilterMaps;
	std::unique_ptr<RenderTarget2D> IBLBrdfLUT2D;

	// Deferred Redenring
	// GBuffer info
	std::unique_ptr<RenderTarget2D> GBufferAlbedo;
	std::unique_ptr<RenderTarget2D> GBufferSpecular;
	std::unique_ptr<RenderTarget2D> GBufferWorldPos;
	std::unique_ptr<RenderTarget2D> GBufferNormal;
	std::unique_ptr<RenderTarget2D> GBufferVelocity;
	std::unique_ptr<D3D12DepthBuffer> GBufferDepth;
	TD3D12RWStructuredBufferRef TileLightInfoListRef;

	std::unique_ptr<D3D12ColorBuffer> TiledDepthDebugTexture;

	// SSAO
	std::unique_ptr<D3D12ColorBuffer> SSAOTexture;
	std::unique_ptr<D3D12ColorBuffer> SSAOBlurTexture;
	TD3D12ConstantBufferRef SSAOCBRef;
	TD3D12StructuredBufferRef ssaoKernelSBRef;
	TD3D12StructuredBufferRef randomSBRef;

	// HBAO
	std::unique_ptr<D3D12ColorBuffer> HBAOTexture;
	std::unique_ptr<D3D12ColorBuffer> HBAOBlurTexture;

	// TAA 
	std::unique_ptr<D3D12ColorBuffer> CacheColorTexture;
	std::unique_ptr<D3D12ColorBuffer> PreColorTexture;

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

	// FXAA 
	std::unique_ptr<D3D12ColorBuffer> FXAATexture;
};

 