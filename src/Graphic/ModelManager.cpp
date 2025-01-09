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
		XMMATRIX scalingMat1 = XMMatrixScaling(scale, scale, scale);
		const XMVECTOR rotationAxisX = XMVectorSet(1, 0, 0, 0);
		XMMATRIX rotationMat = XMMatrixRotationAxis(rotationAxisX, XMConvertToRadians(90));
		ObjCBuffer objCB;
		XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(scalingMat1 * rotationMat));
		wall.SetObjCBuffer(objCB);
		m_ModelMaps["wall"] = wall;
		
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
		Fullquad.CreateQuad(0, 0, 1, 1, 0);
		m_MeshMaps["FullQuad"] = Fullquad;
	}

	void DestroyModel()
	{
		m_ModelMaps.clear();
	}
};


