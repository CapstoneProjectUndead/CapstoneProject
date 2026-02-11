#include "pch.h"
#include "SceneManager.h"
// 서버쪽 SceneManager

#include "TitleScene.h"
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"

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
			if (scene)
			{
				scene->Update(elapsedTime);
			}
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
			if (scene)
			{
				scene->SendResults();
			}
		}
	}
#endif

}

uint32 CSceneManager::CreateRoom(const string& name, shared_ptr<CUser> user)
{
	unique_ptr<CRoom> room = make_unique<CRoom>(name);
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();
	uint32 roomId = room->GetRoomID();

	// 플레이어 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	player->SetID(user->GetUserID());
	player->SetRoomID(roomId);
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);
	user->SetRoomID(roomId);

	// 플레이어 Lobby씬 입장
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY]->EnterScene(player);

	// 방 map에 저장
	rooms[room->GetRoomID()] = std::move(room);

	return roomId;
}
