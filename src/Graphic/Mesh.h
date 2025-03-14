#pragma once
#include "stdafx.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12CommandContext.h"

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

	void DrawMesh(TD3D12CommandContext& gfxContext, UINT instCount = 1)
	{
		gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferRef->GetVBV());
		gfxContext.GetCommandList()->IASetIndexBuffer(&m_indexBufferRef->GetIBV());
		gfxContext.GetCommandList()->DrawIndexedInstanced(m_indices16.size(), instCount, 0, 0, 0);
	}

	void Close()
	{
		// empty
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSRV() { return m_SRV; }
	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetSRV() const { return m_SRV; }

	const std::vector<Vertex>& GetVertices() const { return m_vertices; }
	const std::vector<int16_t>& GetIndices16() const { return m_indices16; }

	const TD3D12VertexBufferRef& GetVertexBuffer() const { return m_vertexBufferRef; }
	const TD3D12IndexBufferRef& GetIndexBuffer() const { return m_indexBufferRef; }

	void SetTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv)
	{
		m_SRV.push_back(srv);
	}

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

private:
	// initialize buffer
	void setupMesh();

	std::vector<Vertex>	m_vertices;
	std::vector<uint32_t> m_indices32;
	std::vector<int16_t> m_indices16;
	std::vector<TD3D12Texture> m_textures;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_SRV;

	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
	ID3D12DescriptorHeap* Heaps;

	// vertex buffer
	TD3D12VertexBufferRef m_vertexBufferRef;
	TD3D12IndexBufferRef  m_indexBufferRef;
};
