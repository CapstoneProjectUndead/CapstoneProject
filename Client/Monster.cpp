#include "stdafx.h"
#include "Monster.h"

CMonster::CMonster()
	: CCharacter(OBJECT_TYPE::MONSTER)
	, AI_state(AI_STATE::MONSTER_IDLE)
{
}

CMonster::~CMonster()
{
}

void CMonster::Update(float elapsedTime)
{
	CCharacter::Update(elapsedTime);
}