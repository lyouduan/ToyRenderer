#pragma once
#include "stdafx.h"
#include "D3D12Buffer.h"
#include "D3D12RHI.h"
#include "D3D12Texture.h"

using namespace DirectX;

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT2 tex;
};

class Mesh
{
public:
	Mesh(std::vector<Vertex>&	vertices, std::vector<int16_t>& indices, std::vector<TD3D12Texture>& textures)
		:m_vertices(vertices), m_indices(indices), m_textures(textures)
	{
		this->setupMesh();
	}

	void DrawMesh(TD3D12CommandContext& gfxContext)
	{
		ID3D12DescriptorHeap* Heaps[] = { m_descriptorCache->GetCacheCbvSrvUavDescriptorHeap().Get() };

		gfxContext.GetCommandList()->SetDescriptorHeaps(1, Heaps);
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_descriptorCache->GetCacheCbvSrvUavDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
		gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(1, srvHandle);

		gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferRef->GetVBV());
		gfxContext.GetCommandList()->IASetIndexBuffer(&m_indexBufferRef->GetIBV());
		gfxContext.GetCommandList()->DrawIndexedInstanced(m_indices.size(), 1, 0, 0, 0);
	}

	void Close()
	{
		// empty
	}

	const std::vector<Vertex>& GetVertices() const { return m_vertices; }
	const std::vector<int16_t>& GetIndices() const { return m_indices; }

	const TD3D12VertexBufferRef& GetVertexBuffer() const { return m_vertexBufferRef; }
	const TD3D12IndexBufferRef& GetIndexBuffer() const { return m_indexBufferRef; }

private:
	// initialize buffer
	void setupMesh()
	{
		m_descriptorCache = std::make_unique<TD3D12DescriptorCache>(TD3D12RHI::g_Device);

		m_vertexBufferRef = TD3D12RHI::CreateVertexBuffer(m_vertices.data(), m_vertices.size() * sizeof(Vertex), sizeof(Vertex));
		m_indexBufferRef = TD3D12RHI::CreateIndexBuffer(m_indices.data(), m_indices.size() * sizeof(int16_t), DXGI_FORMAT_R16_UINT);

		for (auto& tex : m_textures)
		{
			m_SRV.push_back(tex.GetSRV());
		}
		m_descriptorCache->AppendCbvSrvUavDescriptors(m_SRV);
	}

	std::vector<Vertex>	m_vertices;
	std::vector<int16_t> m_indices;
	std::vector<TD3D12Texture> m_textures;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_SRV;
	std::shared_ptr<TD3D12DescriptorCache> m_descriptorCache = nullptr;

	// vertex buffer
	TD3D12VertexBufferRef m_vertexBufferRef;
	TD3D12IndexBufferRef  m_indexBufferRef;
	
};

