#include "stdafx.h"
#include "DogMonster.h"

CDogMonster::CDogMonster()
	: CMonster(MON_TYPE::ANIMAL_MONSTER)
{
}

CDogMonster::~CDogMonster()
{
}

void CDogMonster::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);
}

void CDogMonster::OnIdleMove(float elapsedTime)
{
}

void CDogMonster::OnPatrolMove(float elapsedTime)
{
}

void CDogMonster::OnTraceMove(float elapsedTime)
{
}

void CDogMonster::OnAttackMove(float elapsedTime)
{
}

void CDogMonster::OnIdleEnter()
{
}

void CDogMonster::OnPatrolEnter()
{
}

void CDogMonster::OnTraceEnter()
{
}

void CDogMonster::OnAttackEnter()
{
}
