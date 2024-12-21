#include "ModelLoader.h"
#include "WICTextureLoader.h"

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

void ModelLoader::Draw(TD3D12CommandContext& gfxContext)
{
	for (auto& mesh : m_meshes)
	{
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

		constexpr unsigned int uvIndex = 0;
		if (mesh->HasTextureCoords(uvIndex))
		{
			vertex.tex.x = (float)mesh->mTextureCoords[uvIndex][i].x;
			vertex.tex.y = (float)mesh->mTextureCoords[uvIndex][i].y;
		}
		else {
			vertex.tex = { 0.0f, 0.0f };
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

		//std::vector<TD3D12Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
		//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
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
			TD3D12Texture texture;

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
				texture.LoadImageFromFile(std::string(filenames.begin(), filenames.end()), 0, false);
			}

			texture.name = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}

	return textures;
}

TD3D12Texture ModelLoader::loadEmbeddedTexture(const aiTexture* embeddedTexture)
{
	HRESULT hr;

	TD3D12Texture texture;

	if (embeddedTexture->mHeight != 0)
	{
		TextureInfo info;
		info.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		info.Width = embeddedTexture->mWidth;
		info.Height = embeddedTexture->mHeight;
		info.DepthOrArraySize = 1;
		info.MipCount = 1;
		info.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		info.InitState = D3D12_RESOURCE_STATE_COPY_DEST;
		TD3D12Texture texture;
		texture.Create2D(info);

		D3D12_SUBRESOURCE_DATA subresourceData;
		subresourceData.pData = embeddedTexture->pcData;
		subresourceData.RowPitch = embeddedTexture->mWidth * 4;
		subresourceData.SlicePitch = embeddedTexture->mWidth * embeddedTexture->mHeight * 4;

		TD3D12Resource DestTexture(texture.GetResource()->D3DResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
		TD3D12RHI::InitializeTexture(DestTexture, 1, &subresourceData);

		return texture;
	}

	// mHeight is 0, so try to load a compressed texture of mWidth bytes
	//const size_t size = embeddedTexture->mWidth;
	//hr = CreateWICTextureFromMemory(size, reinterpret_cast<const unsigned char*>(embeddedTexture->pcData), texture);


	return texture;
}

