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

	if (melee_knockback_timer > 0.0f) {
		float ratio = melee_knockback_timer / MELEE_KNOCKBACK_DURATION;
		velocity.x = melee_knockback_vel.x * ratio;
		velocity.z = melee_knockback_vel.z * ratio;
		melee_knockback_timer -= elapsedTime;
		if (melee_knockback_timer < 0.0f) melee_knockback_timer = 0.0f;
	}
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
