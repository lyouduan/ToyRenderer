#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "D3D12CommandContext.h"
#include <unordered_map>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

class Mesh;

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	bool Load(std::string fileName);
	void Draw(TD3D12CommandContext& gfxContext);

	void Close();

private:
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<TD3D12Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);

	TD3D12Texture loadEmbeddedTexture(const aiTexture* embeddedTexture);

	std::string m_directory;
	std::vector<Mesh> m_meshes;
	std::vector<TD3D12Texture> textures_loaded;
};

