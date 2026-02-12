#include "pch.h"
#include "Room.h"
#include "Scene.h"


atomic<uint64> CRoom::s_room_id_generator = 1;

CRoom::CRoom(string name)
	: room_id(s_room_id_generator++)
	, room_name(name)
	, total_player(0)
{
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