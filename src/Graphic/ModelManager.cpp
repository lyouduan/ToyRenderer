#include "ModelManager.h"

namespace ModelManager
{
	std::unordered_map<std::string, ModelLoader> m_ModelMaps;

	void LoadModel()
	{
		ModelLoader nanosuit;
		if (!nanosuit.Load("./models/nanosuit/nanosuit.obj"))
			assert(false);

		m_ModelMaps["nanosuit"] = nanosuit;

		ModelLoader wall;
		if (!wall.Load("./models/brick_wall/brick_wall.obj"))
			assert(false);

		float scale = 10;
		XMMATRIX scalingMat = XMMatrixScaling(scale, scale, scale);
		const XMVECTOR rotationAxis = XMVectorSet(1, 0, 0, 0);
		XMMATRIX rotationMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(90));

		ObjCBuffer obj;
		XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(scalingMat * rotationMat));
		wall.SetObjCBuffer(obj);
		m_ModelMaps["wall"] = wall;
		
	}

	void DestroyModel()
	{
		m_ModelMaps.clear();
	}
};


