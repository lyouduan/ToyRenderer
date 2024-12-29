#pragma once
#include "stdafx.h"
#include "D3D12Buffer.h"
#include "D3D12RHI.h"
#include "D3D12Texture.h"

using namespace DirectX;

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT2 tex;
};

class Mesh
{
public:
	Mesh() {}

	Mesh(std::vector<Vertex>&	vertices, std::vector<int16_t>& indices, std::vector<TD3D12Texture>& textures)
		:m_vertices(vertices), m_indices16(indices), m_textures(textures)
	{
		this->setupMesh();
	}

	void DrawMesh(TD3D12CommandContext& gfxContext)
	{
		//if (!m_SRV.empty())
		//{
			//gfxContext.GetCommandList()->SetDescriptorHeaps(1, &Heaps);
			//D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_descriptorCache->GetCacheCbvSrvUavDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
			//gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(1, m_GpuHandle);
		//}

		gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferRef->GetVBV());
		gfxContext.GetCommandList()->IASetIndexBuffer(&m_indexBufferRef->GetIBV());
		gfxContext.GetCommandList()->DrawIndexedInstanced(m_indices16.size(), 1, 0, 0, 0);
	}

	void Close()
	{
		// empty
	}

	std::shared_ptr<TD3D12DescriptorCache> GetTD3D12DescriptorCache() { return m_descriptorCache; }

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSRV() { return m_SRV; }
	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSRV() const { return m_SRV; }

	const std::vector<Vertex>& GetVertices() const { return m_vertices; }
	const std::vector<int16_t>& GetIndices16() const { return m_indices16; }

	const TD3D12VertexBufferRef& GetVertexBuffer() const { return m_vertexBufferRef; }
	const TD3D12IndexBufferRef& GetIndexBuffer() const { return m_indexBufferRef; }

public:
	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	void CreateBox(float width, float height, float depth, uint32_t numSubdivisions);

	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	void CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);

	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	void CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);

	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	void CreateGrid(float width, float depth, uint32_t m, uint32_t n);

	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	void CreateQuad(float x, float y, float w, float h, float depth);

	void GenerateIndices16();

private:
	// initialize buffer
	void setupMesh()
	{
		m_descriptorCache = std::make_unique<TD3D12DescriptorCache>(TD3D12RHI::g_Device);

		m_vertexBufferRef = TD3D12RHI::CreateVertexBuffer(m_vertices.data(), m_vertices.size() * sizeof(Vertex), sizeof(Vertex));
		m_indexBufferRef = TD3D12RHI::CreateIndexBuffer(m_indices16.data(), m_indices16.size() * sizeof(int16_t), DXGI_FORMAT_R16_UINT);

		for (auto& tex : m_textures)
		{
			m_SRV.push_back(tex.GetSRV());
		}
		//if (!m_SRV.empty())
		//{
		//	m_GpuHandle = m_descriptorCache->AppendCbvSrvUavDescriptors(m_SRV);
		//
		//	Heaps = { m_descriptorCache->GetCacheCbvSrvUavDescriptorHeap().Get() };
		//}
	}

	std::vector<Vertex>	m_vertices;
	std::vector<uint32_t> m_indices32;
	std::vector<int16_t> m_indices16;
	std::vector<TD3D12Texture> m_textures;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_SRV;
	std::shared_ptr<TD3D12DescriptorCache> m_descriptorCache = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
	ID3D12DescriptorHeap* Heaps;

	// vertex buffer
	TD3D12VertexBufferRef m_vertexBufferRef;
	TD3D12IndexBufferRef  m_indexBufferRef;
};

