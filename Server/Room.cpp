#include "pch.h"
#include "Room.h"
#include "Scene.h"


atomic<uint32> CRoom::s_room_id_generator = 1;

CRoom::CRoom(RoomInfo roomInfo)
	: room_info{}
{
	room_info.room_id = s_room_id_generator++;
	strncpy_s(room_info.room_name, roomInfo.room_name, ROOM_NAME_MAX - 1);
	room_info.current_player_count = roomInfo.current_player_count;
	room_info.is_game_start = roomInfo.is_game_start;
}

CRoom::CRoom(string name)
	: room_info{} // 일단 0으로 깨끗하게 밀어버림
{
	room_info.room_id = s_room_id_generator++;
	strncpy_s(room_info.room_name, name.c_str(), ROOM_NAME_MAX - 1);
	room_info.current_player_count = 1;
	room_info.is_game_start = false;
}

CRoom::~CRoom()
{
}

void CRoom::Update(const float elapsedTime)
{
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
	for (auto& scene : scenes)
	{
		if (scene)
		{
			scene->SendResults();
		}
	}
}

bool CRoom::SearchPlayersAllScene()
{
	for (auto& scene : scenes)
	{
		if (scene)
		{
			if (scene->HasPlayers())
				return false;
		}
	}

	return true;
}
