#include "stdafx.h"
#include "AttackState.h"
#include "AIComponent.h"
#include "Monster.h"

CAttackState::CAttackState()
	: CState(MON_STATE::ATTACK)
{
}

CAttackState::~CAttackState()
{
}

void CAttackState::Update(float elapsedTime)
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnAttackMove(elapsedTime);
	}
	break;
	default:
		break;
	}
}

void CAttackState::Enter()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		// 몬스터에게 공격 상태 진입을 알림
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnAttackEnter();
	}
	break;
	default:
		break;
	}
}

void CAttackState::Exit()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnAttackExit();
	}
	break;
	default:
		break;
	}
}
