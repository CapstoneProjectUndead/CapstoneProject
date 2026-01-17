#include "pch.h"
#include "Object.h"
#include "Player.h"

atomic<int> CObject::s_idGenerator = 1;

CObject::CObject()
	: obj_id(-1)
{

}

CObject::~CObject()
{

}

shared_ptr<CPlayer> CObject::CreatePlayer()
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	player->SetID(s_idGenerator);
	++s_idGenerator;

	return player;
}

void CObject::Update(float elapsedTime)
{

}
