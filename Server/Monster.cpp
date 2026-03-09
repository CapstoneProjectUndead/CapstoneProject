#include "pch.h"
// ServerÂÊ Monster
#include "Monster.h"

CMonster::CMonster(MON_TYPE type)
	: CObject(OBJECT_TYPE::MONSTER)
	, monster_type(type)
	, AI_state(AI_STATE::MONSTER_IDLE)
{
}

CMonster::~CMonster()
{
}

void CMonster::Update(float elapsedTime)
{
	last_simulated_time = static_cast<float>(g_server_total_time);

	CObject::Update(elapsedTime);
}