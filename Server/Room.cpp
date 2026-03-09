#include "pch.h"
#include "Room.h"
#include "Scene.h"
#include "CustomScene.h"
#include "LobbyScene.h"

#define ROOM_MAX_PLAYER 4

atomic<uint32> CRoom::s_room_id_generator = 1;

CRoom::CRoom(RoomInfo roomInfo)
	: room_info{}
	, is_active(true)
{
	room_info.room_id = s_room_id_generator++;
	strncpy_s(room_info.room_name, roomInfo.room_name, ROOM_NAME_MAX - 1);
	room_info.current_player_count = roomInfo.current_player_count;
	room_info.is_game_start = roomInfo.is_game_start;
}

CRoom::CRoom(string name)
	: room_info{} 
	, is_active(true)
{
	room_info.room_id = s_room_id_generator++;
	strncpy_s(room_info.room_name, name.c_str(), ROOM_NAME_MAX - 1);
	room_info.current_player_count = 0;
	room_info.is_game_start = false;
}

CRoom::~CRoom()
{
	
}

void CRoom::Initialize()
{
	// CCustomScene 생성
	scenes[(UINT)SCENE_TYPE::CUSTOMS] = make_unique<CCustomScene>(room_info.room_id);
	scenes[(UINT)SCENE_TYPE::CUSTOMS]->SetRoom(shared_from_this());
	scenes[(UINT)SCENE_TYPE::CUSTOMS]->Start();

	// LobbyScene 생성
	scenes[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>(room_info.room_id);
	scenes[(UINT)SCENE_TYPE::LOBBY]->SetRoom(shared_from_this());
	scenes[(UINT)SCENE_TYPE::LOBBY]->Start();

	// 나중에 여기서 GameScene도 같이 생성
}

void CRoom::Update(const float elapsedTime)
{
	lock_guard<mutex> lg(room_lock);

	if (!is_active)
		return;

	for (auto& scene : scenes)
	{
		if (scene)
		{
			scene->Update(elapsedTime);
		}
	}
}

void CRoom::SendResults()
{
	lock_guard<mutex> lg(room_lock);

	if (!is_active)
		return;

	for (auto& scene : scenes)
	{
		if (scene)
		{
			scene->SendResults();
		}
	}
}

bool CRoom::IsValid()
{
	RoomInfo info = GetRoomInfo();

	// 방 인원이 꽉 찼으면 false
	if (info.current_player_count >= ROOM_MAX_PLAYER)
		return false;

	// 게임이 시작된 방이면 false
	if (info.is_game_start)
		return false;
	
	return true;
}