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

void CRoomManager::Initialize()
{
	// 임시
	{
		RoomInfo info{};
		info.current_player_count = 1;
		info.is_game_start = false;
		info.room_id = 1;
		COPY_STRING(info.room_name, "테스트 Room");

		unique_ptr<CRoom> room = make_unique<CRoom>(info);
		rooms[info.room_id] = std::move(room);
	}
}

void CRoomManager::Update(const float elapsedTime)
{
	lock_guard<mutex> lg(rooms_lock);
	for (auto& [id, room] : rooms)
	{
		room->Update(elapsedTime);
	}
}

void CRoomManager::SendResults()
{
	lock_guard<mutex> lg(rooms_lock);
	for (auto& [id, room] : rooms)
	{
		room->SendResults();
	}
}

uint32 CRoomManager::CreateRoom(const string& name, shared_ptr<CUser> user)
{
	unique_ptr<CRoom> room = make_unique<CRoom>(name);

	uint32 roomId = room->GetRoomID();
	NetRoomInfo info{ room->GetRoomInfo() };
	auto session = user->GetSession();
	assert(session);

	// 플레이어 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어의 (고유ID = 유저ID)
	player->SetID(user->GetUserID());

	// 지금 플레이어가 속한 방ID
	player->SetRoomID(roomId);

	// 지금 플레이어가 속한 Scene
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 유저에도 자신이 속한 방ID를 가지고 있는다.
	user->SetRoomID(roomId);

	// LobbyScene 생성
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();

	// 플레이어 Lobby씬 입장
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY]->EnterScene(player);

	// 방 map에 저장
	lock_guard<mutex> lg(rooms_lock);
	rooms[room->GetRoomID()] = std::move(room);

	// 유저에게 방 생성을 허락.
	S_CreateRoom createRoomPkt;
	createRoomPkt.room_info = info;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_CreateRoom>(createRoomPkt);
	session->DoSend(sendBuffer);

	return roomId;
}