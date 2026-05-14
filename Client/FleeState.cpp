#include "stdafx.h"
#include "FleeState.h"
#include "AIComponent.h"
#include "Monster.h"

CFleeState::CFleeState()
	: CState(AI_STATE::MONSTER_FLEE)
{

}

CFleeState::~CFleeState()
{
}

void CFleeState::Update(float elapsedTime)
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnFleeMove(elapsedTime);
	}
	break;
	default:
		break;
	}
}

void CFleeState::Enter()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		// 몬스터에게 공격 상태 진입을 알림
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->SetAIState(AI_STATE::MONSTER_FLEE);
		monster->OnFleeEnter();
	}
	break;
	default:
		break;
	}
}

void CFleeState::Exit()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnFleeExit();
	}
	break;
	default:
		break;
	}
}
