#include "Mesh.h"
#include "GeometryGenerator.h"

void Mesh::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geo = geoGen.CreateBox(width, height, depth, numSubdivisions);
	
	m_vertices.resize(geo.Vertices.size());
	for (size_t i = 0; i < geo.Vertices.size(); ++i)
	{
		m_vertices[i].position = geo.Vertices[i].Position;
		m_vertices[i].tangent = geo.Vertices[i].TangentU;
		m_vertices[i].normal = geo.Vertices[i].Normal;
		m_vertices[i].tex = geo.Vertices[i].TexC;
	}

	m_indices16.assign(geo.GetIndices16().begin(), geo.GetIndices16().end());

	m_indices32.assign(geo.Indices32.begin(), geo.Indices32.end());

	this->setupMesh();
}

void Mesh::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{

	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geo = geoGen.CreateSphere(radius, sliceCount, stackCount);

	m_vertices.resize(geo.Vertices.size());
	for (size_t i = 0; i < geo.Vertices.size(); ++i)
	{
		m_vertices[i].position = geo.Vertices[i].Position;
		m_vertices[i].tangent = geo.Vertices[i].TangentU;
		m_vertices[i].normal = geo.Vertices[i].Normal;
		m_vertices[i].tex = geo.Vertices[i].TexC;
	}

	m_indices16.assign(geo.GetIndices16().begin(), geo.GetIndices16().end());

	m_indices32.assign(geo.Indices32.begin(), geo.Indices32.end());

	this->setupMesh();
}

void Mesh::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geo = geoGen.CreateCylinder(bottomRadius, topRadius, height, sliceCount, stackCount);

	m_vertices.resize(geo.Vertices.size());
	for (size_t i = 0; i < geo.Vertices.size(); ++i)
	{
		m_vertices[i].position = geo.Vertices[i].Position;
		m_vertices[i].tangent = geo.Vertices[i].TangentU;
		m_vertices[i].normal = geo.Vertices[i].Normal;
		m_vertices[i].tex = geo.Vertices[i].TexC;
	}

	m_indices16.assign(geo.GetIndices16().begin(), geo.GetIndices16().end());

	m_indices32.assign(geo.Indices32.begin(), geo.Indices32.end());

	this->setupMesh();
}

void Mesh::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geo = geoGen.CreateGrid(width, depth, m, n);

	m_vertices.resize(geo.Vertices.size());
	for (size_t i = 0; i < geo.Vertices.size(); ++i)
	{
		m_vertices[i].position = geo.Vertices[i].Position;
		m_vertices[i].tangent = geo.Vertices[i].TangentU;
		m_vertices[i].normal = geo.Vertices[i].Normal;
		m_vertices[i].tex = geo.Vertices[i].TexC;
	}

	m_indices16.assign(geo.GetIndices16().begin(), geo.GetIndices16().end());

	m_indices32.assign(geo.Indices32.begin(), geo.Indices32.end());

	this->setupMesh();
}

void Mesh::CreateQuad(float x, float y, float w, float h, float depth)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geo = geoGen.CreateQuad(x,y,w,h, depth);

	m_vertices.resize(geo.Vertices.size());
	for (size_t i = 0; i < geo.Vertices.size(); ++i)
	{
		m_vertices[i].position = geo.Vertices[i].Position;
		m_vertices[i].tangent = geo.Vertices[i].TangentU;
		m_vertices[i].normal = geo.Vertices[i].Normal;
		m_vertices[i].tex = geo.Vertices[i].TexC;
	}

	m_indices16.assign(geo.GetIndices16().begin(), geo.GetIndices16().end());

	m_indices32.assign(geo.Indices32.begin(), geo.Indices32.end());

	this->setupMesh();
}
