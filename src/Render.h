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
	void CreateIBLPrefilterMap();


	std::unique_ptr<SceneCaptureCube>& GetIBLEnvironmemtMap() { return IBLEnvironmentMap; }
	std::unique_ptr<SceneCaptureCube>& GetIBLIrradianceMap() { return IBLIrradianceMap; }
	std::unique_ptr<SceneCaptureCube>& GetIBLPrefilterMaps(int i) 
	{ 
		assert(i >= 0 && i < IBLPrefilterMaxMipLevel);

		return IBLPrefilterMaps[i]; 
	}
	std::vector<std::unique_ptr<SceneCaptureCube>>& GetIBLPrefilterMaps() { return IBLPrefilterMaps; }

	bool GetEnableIBLEnvLighting() { return bEnableIBLEnvLighting; }
	void SetEnableIBLEnvLighting(bool b) { bEnableIBLEnvLighting = b; }

private:

	void CreateSceneCaptureCube();

private:

	// PBR and IBL
	const static UINT IBLPrefilterMaxMipLevel = 5;

	std::unique_ptr<SceneCaptureCube> IBLEnvironmentMap;
	std::unique_ptr<SceneCaptureCube> IBLIrradianceMap;
	std::vector<std::unique_ptr<SceneCaptureCube>> IBLPrefilterMaps;

	bool bEnableIBLEnvLighting = false;
};

 