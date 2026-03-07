#include "pch.h"
// ServerÂÊ IdleState
#include "IdleState.h"
#include "AIComponent.h"
#include "Monster.h"

CIdleState::CIdleState()
	: CState(AI_STATE::MONSTER_IDLE)
{
}

CIdleState::~CIdleState()
{
}

void CIdleState::Update(float elapsedTime)
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnIdleMove(elapsedTime);
	}
		break;
	default:
		break;
	}
}

void CIdleState::Enter()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->SetAIState(AI_STATE::MONSTER_IDLE);
		monster->OnIdleEnter();
	}
	break;
	default:
		break;
	}
}

void CIdleState::Exit()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnIdleExit();
	}
	break;
	default:
		break;
	}
}
