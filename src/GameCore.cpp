#include "GameCore.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12RHI.h"
#include "D3D12Texture.h"
#include "Display.h"
#include <chrono>

using namespace TD3D12RHI;

GameCore::GameCore(uint32_t width, uint32_t height, std::wstring name)
	:DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f,0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0,0, static_cast<LONG>(width), static_cast<LONG>(height))
{
	g_DisplayWidth = width;
	g_DisplayHeight = height;
}

void GameCore::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void GameCore::OnUpdate(const GameTimer& gt)
{
	totalTime += gt.DeltaTime() * 0.001;
	float angle = static_cast<float>(totalTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

	m_Camera.SetLookAt(eyePosition, focusPoint, upDirection);

	//m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = GetWidth() / static_cast<float>(GetHeight());
	m_Camera.SetAspectRatio(aspectRatio);
	//m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0), aspectRatio, 0.1f, 100.0f);
}

void GameCore::OnRender()
{
	// record all the commands we need to render the scene into the command list
	PopulateCommandList();

	// execute the command list
	g_CommandContext.ExecuteCommandLists();

	// present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
} 

void GameCore::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();
}

void GameCore::LoadPipeline()
{
	uint32_t dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);
		
		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	TD3D12RHI::g_Device = m_device.Detach();

	// using device to initialize CommandContext and allocator
	TD3D12RHI::Initialze();

	descriptorCache = std::make_unique<TD3D12DescriptorCache>(g_Device);

	// create the swap chain
	Display::Initialize();

	// this sample does not support fullscreen transition
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(TD3D12RHI::g_SwapCHain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// create frame resources
	{
		for(uint32_t n = 0; n < FrameCount; ++n)
		{
			ComPtr<ID3D12Resource> resource = nullptr;
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&resource)));
			m_renderTragetrs[n].CreateFromSwapChain(L"Render Target", resource.Detach());
		}
	}
}

// load the sample assets
void GameCore::LoadAssets()
{
	// create an empty root signature
	{
		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_DESCRIPTOR_RANGE1 srvRange;
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &srvRange);

		// sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(TD3D12RHI::g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	// create the pipeline state, which includes compiling and loading shaders
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// enable better shader debugging with the graphics debugging tools
		uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		uint32_t compileFlags = 0;
#endif // defined(_DEBUG)

		ThrowIfFailed(D3DCompileFromFile(L"shaders/shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders/shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));


		// define  the vertex input layout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		// describe and create the graphics pipeline state object (PSO)
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(TD3D12RHI::g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	// create the vertex buffer
	{
		// define  the geometry for a triangle
		Vertex triangleVerties[] =
		{   { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }, // 0
			{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) }, // 1
			{ XMFLOAT3(1.0f,  1.0f, -1.0f),  XMFLOAT2(0.0f, 1.0f) }, // 2
			{ XMFLOAT3(1.0f, -1.0f, -1.0f),  XMFLOAT2(1.0f, 0.0f) }, // 3
			{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT2(0.0f, 1.0f) }, // 4
			{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT2(1.0f, 0.0f) }, // 5
			{ XMFLOAT3(1.0f,  1.0f,  1.0f),  XMFLOAT2(0.0f, 0.0f) }, // 6
			{ XMFLOAT3(1.0f, -1.0f,  1.0f),  XMFLOAT2(1.0f, 1.0f) }  // 7
		};

		int16_t indices[36] =
		{
			0, 1, 2, 0, 2, 3,
			4, 6, 5, 4, 7, 6,
			4, 5, 1, 4, 1, 0,
			3, 2, 6, 3, 6, 7,
			1, 5, 6, 1, 6, 2,
			4, 0, 3, 4, 3, 7
		};
	
		const uint32_t vertexBufferSize = sizeof(triangleVerties);
		vertexBufferRef = TD3D12RHI::CreateVertexBuffer(&triangleVerties, vertexBufferSize, sizeof(Vertex));

		const uint32_t indexBufferSize = sizeof(indices);
		indexBufferRef = TD3D12RHI::CreateIndexBuffer(&indices, indexBufferSize, DXGI_FORMAT_R16_UINT);
	}

	// load texture data
	TD3D12Texture texture(64, 64, DXGI_FORMAT_R16G16B16A16_FLOAT);
	
	if(texture.CreateDDSFromFile(L"D:/gcRepo/DX12Lab/DX12Lab/textures/Wood.dds", 0, false))
		m_SRV.push_back(texture.GetSRV());

	g_CommandContext.FlushCommandQueue();

	descriptorCache->AppendCbvSrvUavDescriptors(m_SRV);
}

void GameCore::PopulateCommandList()
{
	// reset CommandAllocator and CommandList
	g_CommandContext.ResetCommandAllocator();
	g_CommandContext.ResetCommandList();

	// set necessary state
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
	g_CommandContext.GetCommandList()->RSSetViewports(1, &m_viewport);
	g_CommandContext.GetCommandList()->RSSetScissorRects(1, &m_scissorRect);

	// indicate that back buffer will be used as a render target
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTragetrs[m_frameIndex].GetResource(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier);

	auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_DepthBuffer.GetResource(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier1);

	// indicate that back buffer will be used as a render target
	g_CommandContext.GetCommandList()->ClearDepthStencilView(TD3D12RHI::g_DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

	g_CommandContext.GetCommandList()->OMSetRenderTargets(1, &m_renderTragetrs[m_frameIndex].GetRTV(), TRUE, &TD3D12RHI::g_DepthBuffer.GetDSV());

	// Record commands
	const float clearColor[] = { 0.0, 0.2, 0.4, 1.0 };
	g_CommandContext.GetCommandList()->ClearRenderTargetView(m_renderTragetrs[m_frameIndex].GetRTV(), clearColor, 0, nullptr);

	g_CommandContext.GetCommandList()->SetPipelineState(m_pipelineState.Get());
	//m_commandList->ClearDepthStencilView(dsvhandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_Camera.GetViewProjMat());
	//XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_Camera.GetViewMat());
	//mvpMatrix = XMMatrixMultiply(mvpMatrix, m_Camera.GetProjMat());

	g_CommandContext.GetCommandList()->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	ID3D12DescriptorHeap* Heaps[] = { descriptorCache->GetCacheCbvSrvUavDescriptorHeap().Get() };

	g_CommandContext.GetCommandList()->SetDescriptorHeaps(1, Heaps);
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = descriptorCache->GetCacheCbvSrvUavDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	g_CommandContext.GetCommandList()->SetGraphicsRootDescriptorTable(1, srvHandle);

	g_CommandContext.GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_CommandContext.GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferRef->GetVBV());
	g_CommandContext.GetCommandList()->IASetIndexBuffer(&indexBufferRef->GetIBV());
	g_CommandContext.GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// indicate that the back buffer will now be used to present
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTragetrs[m_frameIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier);

	barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_DepthBuffer.GetResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_COMMON);
	g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier1);
	
	g_CommandContext.FlushCommandQueue();
}

void GameCore::WaitForPreviousFrame()
{
	
	// Signal and increment the fence value
	//const uint64_t fence = ++m_fenceValue;
	//ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	//
	//// wait until the previous frame is finished
	//if (m_fence->GetCompletedValue() < fence)
	//{
	//	ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
	//	WaitForSingleObject(m_fenceEvent, INFINITE);
	//}
	g_CommandContext.FlushCommandQueue();
	
	DescriptorCache->Reset();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
