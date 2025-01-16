#include "ModelManager.h"
#include "RenderInfo.h"

namespace ModelManager
{
	std::unordered_map<std::string, ModelLoader> m_ModelMaps;
	std::unordered_map<std::string, Mesh> m_MeshMaps;

	void LoadModel()
	{
		ModelLoader nanosuit;
		if (!nanosuit.Load("./models/nanosuit/nanosuit.obj"))
			assert(false);
		m_ModelMaps["nanosuit"] = nanosuit;


		ModelLoader wall;
		if (!wall.Load("./models/brick_wall/brick_wall.obj"))
			assert(false);

		float scale = 5;
		XMMATRIX scalingMat = XMMatrixScaling(scale, scale, scale);
		const XMVECTOR rotationAxisX = XMVectorSet(1, 0, 0, 0);
		XMMATRIX rotationMat = XMMatrixRotationAxis(rotationAxisX, XMConvertToRadians(90));
		ObjCBuffer objCB;
		XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(scalingMat * rotationMat));
		wall.SetObjCBuffer(objCB);
		m_ModelMaps["wall"] = wall;


		ModelLoader Cerberus_LP;
		if (!Cerberus_LP.Load("./models/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX"))
			assert(false);

		scalingMat = XMMatrixScaling(0.05, 0.05, 0.05);
		auto rotationAxisY = XMVectorSet(0, 1, 0, 0);
		auto rotationMatY = rotationMat *  XMMatrixRotationAxis(rotationAxisY, XMConvertToRadians(-45));
		XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(rotationMatY * scalingMat));
		Cerberus_LP.SetObjCBuffer(objCB);
		m_ModelMaps["Cerberus_LP"] = Cerberus_LP;
		
	}

	void LoadMesh()
	{
		Mesh box;
		box.CreateBox(1, 1, 1, 3);
		m_MeshMaps["box"] = box;

		Mesh sphere;
		sphere.CreateSphere(1, 20, 20);
		m_MeshMaps["sphere"] = sphere;
		
		Mesh Fullquad;
		Fullquad.CreateQuad(-1, 1, 2, 2, 0);
		m_MeshMaps["FullQuad"] = Fullquad;
	}

	void DestroyModel()
	{
		m_ModelMaps.clear();
	}
};


