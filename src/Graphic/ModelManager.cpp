#include "ModelManager.h"
#include "RenderInfo.h"

namespace ModelManager
{
	std::unordered_map<std::string, ModelLoader> m_ModelMaps;
	std::unordered_map<std::string, Mesh> m_MeshMaps;

	void LoadMesh()
	{
		Mesh box;
		box.CreateBox(1, 1, 1, 3);
		m_MeshMaps["box"] = box;


		ModelLoader boxModel;
		boxModel.SetMesh(m_MeshMaps["box"]);
		m_ModelMaps["box"] = std::move(boxModel);

		Mesh sphere;
		sphere.CreateSphere(1, 20, 20);
		m_MeshMaps["sphere"] = sphere;

		Mesh pointLight;
		pointLight.CreateSphere(1, 20, 20);
		m_MeshMaps["SpotLight"] = pointLight;

		ModelLoader sphereModel;
		sphereModel.SetMesh(m_MeshMaps["sphere"]);
		m_ModelMaps["sphere"] = std::move(sphereModel);

		Mesh Fullquad;
		Fullquad.CreateQuad(-1, 1, 2, 2, 0);
		m_MeshMaps["FullQuad"] = Fullquad;

		ModelLoader FullquadModel;
		FullquadModel.SetMesh(m_MeshMaps["FullQuad"]);
		m_ModelMaps["FullQuad"] = std::move(FullquadModel);

		Mesh DebugQuad;
		DebugQuad.CreateQuad(0.5, -0.5, 0.5, 0.5, 0);
		m_MeshMaps["DebugQuad"] = DebugQuad;

		ModelLoader DebugQuadquadModel;
		DebugQuadquadModel.SetMesh(m_MeshMaps["DebugQuad"]);
		m_ModelMaps["DebugQuad"] = std::move(DebugQuadquadModel);

		Mesh HalfQuad;
		HalfQuad.CreateQuad(0, 0, 1, 1, 0);
		m_MeshMaps["HalfQuad"] = HalfQuad;

		Mesh Cylinder;
		Cylinder.CreateCylinder(1, 1, 5, 10, 10);
		m_MeshMaps["Cylinder"] = Cylinder;

		ModelLoader CylinderModel;
		CylinderModel.SetMesh(m_MeshMaps["Cylinder"]);
		m_ModelMaps["Cylinder"] = std::move(CylinderModel);
	}

	void LoadModel()
	{
		LoadMesh();

		ModelLoader nanosuit;
		if (!nanosuit.Load("./models/nanosuit/nanosuit.obj"))
			assert(false);
		m_ModelMaps["nanosuit"] = nanosuit;

		ModelLoader wall;
		if (!wall.Load("./models/brick_wall/brick_wall.obj"))
			assert(false);

		float scale = 50;
		XMMATRIX scalingMat = XMMatrixScaling(scale, scale, scale);
		const XMVECTOR rotationAxisX = XMVectorSet(1, 0, 0, 0);
		XMMATRIX rotationMat = XMMatrixRotationAxis(rotationAxisX, XMConvertToRadians(90));

		auto modelMat = scalingMat * rotationMat;
		auto det = XMMatrixDeterminant(modelMat);
		auto invTranModelMat = XMMatrixTranspose(XMMatrixInverse(&det, modelMat));

		ObjCBuffer objCB;
		XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(modelMat));
		XMStoreFloat4x4(&objCB.InvTranModelMat, XMMatrixTranspose(invTranModelMat));
		wall.SetObjCBuffer(objCB);
		m_ModelMaps["wall"] = wall;


		Mesh gridMesh;
		gridMesh.CreateGrid(100.0, 100.0, 60, 40);
		TD3D12Texture tex;
		tex.Create2D(64, 64);
		if (tex.CreateWICFromFile(L"./textures/grid.jpg", 0, false))
			gridMesh.SetTexture(tex.GetSRV());
		else
			assert(false);
		ModelLoader floor;
		floor.SetMesh(gridMesh);
		m_ModelMaps["floor"] = floor;

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

	

	void DestroyModel()
	{
		m_ModelMaps.clear();
	}
};


