#include "pch.h"
#include "RoomManager.h"
#include "Scene.h"
#include "LobbyScene.h"
#include "User.h"


CRoomManager::CRoomManager()
{
}

CRoomManager::~CRoomManager()
{
}

void CRoomManager::Update(const float elapsedTime)
{
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
}

void CRoomManager::SendResults()
{
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
}

uint32 CRoomManager::CreateRoom(const string& name, shared_ptr<CUser> user)
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