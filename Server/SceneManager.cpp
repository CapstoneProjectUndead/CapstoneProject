#include "pch.h"
#include "SceneManager.h"
// 서버쪽 SceneManager

#include "TitleScene.h"
#include "LobbyScene.h"

atomic<uint64> CRoom::s_room_id_generator = 1;

CRoom::CRoom(string name)
	: room_id(s_room_id_generator++)
	, room_name(name)
{
}

CRoom::~CRoom()
{
}

CSceneManager::CSceneManager()
{

}

CSceneManager::~CSceneManager()
{

}

void CSceneManager::Initialize()
{
#ifdef SCENE_TEST
	scenes[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();
#else
	title_scene = make_unique<CTitleScene>();

	//shared_ptr<CRoom> room = make_shared<CRoom>("초보 환영");
	//room->GetScenes()[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();

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

	for (auto& [id, room] : rooms)
	{
		for (auto& scene : room->GetScenes())
		{
			scene->Update(elapsedTime);
		}
	}
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
	// TitleScene은 항상 결과전송 (뭔가있을때만)
	title_scene->SendResults();

	for (auto& [id, room] : rooms)
	{
		for (auto& scene : room->GetScenes())
		{
			scene->SendResults();
		}
	}
#endif

}

void CSceneManager::CreateRoom(const string& name)
{
	shared_ptr<CRoom> room = make_shared<CRoom>(name);
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();
}
