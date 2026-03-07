#include "pch.h"
// ServerÂÊ TraceState
#include "TraceState.h"
#include "AIComponent.h"
#include "Monster.h"

CTraceState::CTraceState()
	: CState(AI_STATE::MONSTER_TRACE)
{
}

CTraceState::~CTraceState()
{
}

void CTraceState::Update(float elapsedTime)
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnTraceMove(elapsedTime);
	}
	break;
	default:
		break;
	}
}

void CTraceState::Enter()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->SetAIState(AI_STATE::MONSTER_TRACE);
		monster->OnTraceEnter();
	}
	break;
	default:
		break;
	}
}

void CTraceState::Exit()
{
	OBJECT_TYPE type = GetAI()->GetOwner()->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::MONSTER:
	{
		auto monster = static_cast<CMonster*>(GetAI()->GetOwner());
		monster->OnTraceExit();
	}
	break;
	default:
		break;
	}
}
