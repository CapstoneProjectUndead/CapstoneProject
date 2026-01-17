#include "pch.h"
#include "SceneManager.h"
// ¼­¹öÂÊ SceneManager

#include "TestScene.h"


void CSceneManager::Initialize()
{
	scenes[(UINT)SCENE_TYPE::TEST] = std::make_unique<CTestScene>();
	
	active_scene = scenes[(UINT)SCENE_TYPE::TEST].get();
}

void CSceneManager::Update()
{

}
