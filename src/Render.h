#pragma once

#include <memory>
#include <vector>
#include "SceneCaptureCube.h"

class SceneCaptureCube;

class TRender
{
public:
	TRender();
	~TRender();

	void Initalize();

	void CreateIBLEnvironmentMap();

	void CreateIBLIrradianceMap();


	std::unique_ptr<SceneCaptureCube>& GetIBLEnvironmemtMap() { return IBLEnvironmentMap; }
	std::unique_ptr<SceneCaptureCube>& GetIBLIrradianceMap() { return IBLIrradianceMap; }

private:

	void CreateSceneCaptureCube();

private:

	// PBR and IBL
	std::unique_ptr<SceneCaptureCube> IBLEnvironmentMap;
	std::unique_ptr<SceneCaptureCube> IBLIrradianceMap;

	bool bEnableIBLEnvLighting = false;
};

 