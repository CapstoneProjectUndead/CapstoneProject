#include "pch.h"
#include "SceneManager.h"
// 서버쪽 SceneManager

#include "TitleScene.h"
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"


CSceneManager::CSceneManager()
{

}

CSceneManager::~CSceneManager()
{

}

void CSceneManager::Initialize()
{
#ifdef LOBBY_SCENE_TEST
	scenes[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();
#else
	title_scene = make_unique<CTitleScene>();

#endif
}

void CSceneManager::Update(const float elapsedTime)
{

#ifdef SCENE_TEST
	for (auto& scene : scenes)
	{
		if (scene != nullptr)
			scene->Update(elapsedTime);
	}
#else
	// TitleScene은 항상 Update
	title_scene->Update(elapsedTime);
#endif

}

void CSceneManager::SendResults()
{

#ifdef SCENE_TEST
	for (auto& scene : scenes)
	{
		if (scene != nullptr)
			scene->SendResults();
	}
#else
	// TitleScene은 항상 결과전송
	title_scene->SendResults();
#endif

}