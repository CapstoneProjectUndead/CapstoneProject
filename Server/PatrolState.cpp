#include "pch.h"
// Server쪽 PatrolState
#include "PatrolState.h"
#include "AIComponent.h"
#include "Monster.h"

CPatrolState::CPatrolState()
	: CState(AI_STATE::MONSTER_PATROL)
{
}

CPatrolState::~CPatrolState()
{
}

void CPatrolState::Update(float elapsedTime)
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnPatrolMove(elapsedTime);
	}
	break;
	default:
		break;
	}
}

void CPatrolState::Enter()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->SetAIState(AI_STATE::MONSTER_PATROL);
		monster->OnPatrolEnter();
	}
	break;
	default:
		break;
	}
}

void CPatrolState::Exit()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnPatrolExit();
	}
	break;
	default:
		break;
	}
}
