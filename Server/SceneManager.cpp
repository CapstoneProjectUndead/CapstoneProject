#include "pch.h"
#include "SceneManager.h"
// ¼­¹öÂÊ SceneManager

#include "TestScene.h"


void CSceneManager::Initialize()
{
	scenes[(UINT)SCENE_TYPE::TEST] = std::make_unique<CTestScene>();
}

void CSceneManager::Update(float elapsedTime)
{
	for (auto& scene : scenes)
	{
		if (scene != nullptr)
			scene->Update(elapsedTime);
	}
}