#include "pch.h"
#include "Room.h"
#include "Scene.h"


atomic<uint64> CRoom::s_room_id_generator = 1;

CRoom::CRoom(string name)
	: room_id(s_room_id_generator++)
	, room_name(name)
{
}

CRoom::~CRoom()
{
}
