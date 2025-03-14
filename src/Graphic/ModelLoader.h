#pragma once
#include "stdafx.h"
#include "Shader.h"
#include "RenderInfo.h"
#include "D3D12Texture.h"
#include "D3D12CommandContext.h"
#include <unordered_map>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

class Mesh;

class TModelLoader
{
public:
	TModelLoader();
	~TModelLoader();

	bool Load(std::string fileName);
	void Draw(TD3D12CommandContext& gfxContext, TShader* shader, TD3D12ConstantBufferRef& ConstantBufferRef);
	void Close();

	void SetMesh(Mesh& mesh);
	void SetMeshes(std::vector<Mesh>& meshes);

	void SetObjCBuffer(ObjCBuffer objCB) { m_objCB = objCB; }
	ObjCBuffer GetObjCBuffer() const { return m_objCB; }
	
	const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

private:
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<TD3D12Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);

	std::string m_directory;
	std::vector<Mesh> m_meshes;
	std::vector<TD3D12Texture> textures_loaded;

	ObjCBuffer m_objCB;
};

