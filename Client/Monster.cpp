#include "stdafx.h"
#include "Monster.h"

CMonster::CMonster()
	: CCharacter(OBJECT_TYPE::MONSTER)
{
}

CMonster::~CMonster()
{
}

void CMonster::Update(float elapsedTime)
{
	CCharacter::Update(elapsedTime);
}
