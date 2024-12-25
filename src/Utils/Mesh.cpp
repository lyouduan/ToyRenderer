#include "Mesh.h"

void Mesh::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	Vertex v[24];

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = Vertex{ { -w2, -h2, -d2}, { 0.0f, 1.0f} };
	v[1] = Vertex{ { -w2, +h2, -d2}, { 0.0f, 0.0f} };
	v[2] = Vertex{ { +w2, +h2, -d2}, { 1.0f, 0.0f} };
	v[3] = Vertex{ { +w2, -h2, -d2}, { 1.0f, 1.0f} };

	// Fill in the back face vertex data.
	v[4] = Vertex{ { -w2, -h2, +d2}, { 1.0f, 1.0f} };
	v[5] = Vertex{ { +w2, -h2, +d2}, { 0.0f, 1.0f} };
	v[6] = Vertex{ { +w2, +h2, +d2}, { 0.0f, 0.0f} };
	v[7] = Vertex{ { -w2, +h2, +d2}, { 1.0f, 0.0f} };

	// Fill in the top face vertex data.
	v[8]  = Vertex{ { -w2, +h2, -d2}, { 0.0f, 1.0f} };
	v[9]  = Vertex{ { -w2, +h2, +d2}, { 0.0f, 0.0f} };
	v[10] = Vertex{ { +w2, +h2, +d2}, { 1.0f, 0.0f} };
	v[11] = Vertex{ { +w2, +h2, -d2}, { 1.0f, 1.0f} };

	// Fill in the bottom face vertex data.
	v[12] = Vertex{ { -w2, -h2, -d2}, { 1.0f, 1.0f} };
	v[13] = Vertex{ { +w2, -h2, -d2}, { 0.0f, 1.0f} };
	v[14] = Vertex{ { +w2, -h2, +d2}, { 0.0f, 0.0f} };
	v[15] = Vertex{ { -w2, -h2, +d2}, { 1.0f, 0.0f} };

	// Fill in the left face vertex data.
	v[16] = Vertex{ { -w2, -h2, +d2}, { 0.0f, 1.0f} };
	v[17] = Vertex{ { -w2, +h2, +d2}, { 0.0f, 0.0f} };
	v[18] = Vertex{ { -w2, +h2, -d2}, { 1.0f, 0.0f} };
	v[19] = Vertex{ { -w2, -h2, -d2}, { 1.0f, 1.0f} };

	// Fill in the right face vertex data.
	v[20] = Vertex{ { +w2, -h2, -d2}, { 0.0f, 1.0f} };
	v[21] = Vertex{ { +w2, +h2, -d2}, { 0.0f, 0.0f} };
	v[22] = Vertex{ { +w2, +h2, +d2}, { 1.0f, 0.0f} };
	v[23] = Vertex{ { +w2, -h2, +d2}, { 1.0f, 1.0f} };

	m_vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	m_indices32.assign(&i[0], &i[36]);


	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);
	for (uint32_t i = 0; i < numSubdivisions; ++i)
		//Subdivide();

	GenerateIndices16();

	this->setupMesh();
}

void Mesh::Subdivide()
{
	// Save a copy of the input geometry.
	std::vector<Vertex> VerticesCopy = m_vertices;
	std::vector<uint32_t> Indices32Copy = m_indices32;

	m_vertices.resize(0);
	m_indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32_t numTris = (uint32_t)Indices32Copy.size() / 3;
	for (uint32_t i = 0; i < numTris; ++i)
	{
		Vertex v0 = VerticesCopy[Indices32Copy[i * 3 + 0]];
		Vertex v1 = VerticesCopy[Indices32Copy[i * 3 + 1]];
		Vertex v2 = VerticesCopy[Indices32Copy[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		//
		// Add new geometry.
		//

		m_vertices.push_back(v0); // 0
		m_vertices.push_back(v1); // 1
		m_vertices.push_back(v2); // 2
		m_vertices.push_back(m0); // 3
		m_vertices.push_back(m1); // 4
		m_vertices.push_back(m2); // 5

		m_indices32.push_back(i * 6 + 0);
		m_indices32.push_back(i * 6 + 3);
		m_indices32.push_back(i * 6 + 5);

		m_indices32.push_back(i * 6 + 3);
		m_indices32.push_back(i * 6 + 4);
		m_indices32.push_back(i * 6 + 5);

		m_indices32.push_back(i * 6 + 5);
		m_indices32.push_back(i * 6 + 4);
		m_indices32.push_back(i * 6 + 2);

		m_indices32.push_back(i * 6 + 3);
		m_indices32.push_back(i * 6 + 1);
		m_indices32.push_back(i * 6 + 4);
	}
}

Vertex Mesh::MidPoint(const Vertex& v0, const Vertex& v1)
{
	// Compute the midpoints of all the attributes.  Vectors need to be normalized
	// since linear interpolating can make them not unit length.  
	XMFLOAT3 pos = { 0.5f * (v0.position.x + v1.position.x), 0.5f * (v0.position.y + v1.position.y), 0.5f * (v0.position.z + v1.position.z) };
	XMFLOAT2 tex = { 0.5f * (v0.tex.x + v1.tex.x), 0.5f * (v0.tex.y + v1.tex.y) };

	return Vertex{ pos, tex };
}

void Mesh::GenerateIndices16()
{
	if (m_indices16.empty())
	{
		m_indices16.resize(m_indices32.size());
		for (size_t i = 0; i < m_indices32.size(); ++i)
			m_indices16[i] = static_cast<int16_t>(m_indices32[i]);
	}
}