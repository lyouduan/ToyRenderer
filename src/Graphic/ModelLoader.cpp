#include "ModelLoader.h"
#include "WICTextureLoader.h"
#include <wchar.h>
#include "Shader.h"

ModelLoader::ModelLoader()
	: m_meshes()
{
}

ModelLoader::~ModelLoader()
{

}

bool ModelLoader::Load(std::string fileName)
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);
	if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
	{
		return false;
	}
	
	m_directory = fileName.substr(0, fileName.find_last_of("/\\"));

	processNode(pScene->mRootNode, pScene);
	
	return true;
}

void ModelLoader::Draw(TD3D12CommandContext& gfxContext, TShader* shader, TD3D12ConstantBufferRef& ConstantBufferRef)
{
	for (auto& mesh : m_meshes)
	{
		// draw call
		shader->SetParameter("MVPcBuffer", ConstantBufferRef);

		auto m_SRV = mesh.GetSRV();
		shader->SetParameter("diffuseMap", m_SRV[0]);
		shader->SetParameter("normalMap", m_SRV[1]);

		shader->SetDescriptorCache(mesh.GetTD3D12DescriptorCache());
		
		shader->BindParameters();
		mesh.DrawMesh(gfxContext);
	}
}

void ModelLoader::Close()
{
	for (auto& mesh : m_meshes)
	{
		mesh.Close();
	}
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(this->processMesh(mesh, scene));
	}

	for (UINT i = 0; i < node->mNumChildren; i++) {
		this->processNode(node->mChildren[i], scene);
	}
}

Mesh ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<int16_t> indices;
	std::vector<TD3D12Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;

		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		if (mesh->HasNormals())
		{
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
		}

		if (mesh->mTextureCoords[0])
		{
			vertex.tex.x = mesh->mTextureCoords[0][i].x;
			vertex.tex.y = mesh->mTextureCoords[0][i].y;
			/*if (!(vertex.tex.x >= 0.0f && vertex.tex.x <= 1.0f))
			{
				wchar_t buffer[128];
				swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"Warning: Missing UV coordinates for vertex, 、ex.x = %f\n", vertex.tex.x);
				OutputDebugString(buffer);
			}
			
			if (!(vertex.tex.y >= 0.0f && vertex.tex.y <= 1.0f))
			{
				wchar_t buffer[128];
				swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"Warning: Missing UV coordinates for vertex, 、ex.y = %f\n", vertex.tex.y);
				OutputDebugString(buffer);
			}*/
			//assert(vertex.tex.x >= 0.0f && vertex.tex.x <= 1.0f);
			//assert(vertex.tex.y >= 0.0f && vertex.tex.y <= 1.0f);
			if(mesh->HasTangentsAndBitangents())
			{
				vertex.tangent.x = mesh->mTangents[i].x;
				vertex.tangent.y = mesh->mTangents[i].y;
				vertex.tangent.z = mesh->mTangents[i].z;
			}
			
		}
		else 
		{
			vertex.tex = { 0.0 ,0.0 };
		}
		
		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// texture
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<TD3D12Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<TD3D12Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

		std::vector<TD3D12Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	}

	return Mesh(vertices, indices, textures);
}

std::vector<TD3D12Texture> ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
{
	std::vector<TD3D12Texture> textures;

	for (UINT i = 0; i < mat->GetTextureCount(type); ++i)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// chech if textures was loaded before and if so, continue to next iteration : skip loading a new texture
		bool skip = false;
		for (UINT j = 0; j < textures_loaded.size(); j++) {
			if (std::strcmp(textures_loaded[j].name.c_str(), str.C_Str()) == 0) {
				textures.push_back(textures_loaded[j]);
				skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if (!skip) {   // If texture hasn't been loaded already, load it
			TD3D12Texture texture(64, 64);

			std::string filename = std::string(str.C_Str());
			filename = m_directory + '/' + filename;
			
			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
			if (embeddedTexture != nullptr)
			{
				//texture = loadEmbeddedTexture(embeddedTexture);
			}
			else
			{
				std::wstring filenames = std::wstring(filename.begin(), filename.end());
				if (filename.find("dds") != std::string::npos)
				{
					texture.CreateDDSFromFile(filenames.c_str(), 0, false);
				}
				else if (filename.find("png") != std::string::npos || filename.find("jpg") != std::string::npos)
				{
					texture.CreateWICFromFile(filenames.c_str(), 0, false);
				}
			}

			texture.name = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}

	return textures;
}
