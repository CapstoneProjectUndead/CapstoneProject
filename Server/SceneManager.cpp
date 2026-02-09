#include "pch.h"
#include "SceneManager.h"
// ¼­¹öÂÊ SceneManager

#include "TitleScene.h"
#include "LobbyScene.h"


void CSceneManager::Initialize()
{
	scenes[(UINT)SCENE_TYPE::TITLE] = std::make_unique<CTitleScene>();
	scenes[(UINT)SCENE_TYPE::LOBBY] = std::make_unique<CLobbyScene>();
}

void CSceneManager::Update(const float elapsedTime)
{
	for (auto& scene : scenes)
	{
		if (scene != nullptr)
			scene->Update(elapsedTime);
	}

	//for (auto& [id, room] : rooms) 
	//{
	//	for (auto& scene : room->scenes) 
	// {
	//		scene->Update(elapsedTime);
	//	}
	//}
}

void CSceneManager::SendResults()
{
	for (auto& scene : scenes)
	{
		if (scene != nullptr)
			scene->SendResults();
	}
}