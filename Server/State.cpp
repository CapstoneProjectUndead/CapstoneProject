#include "pch.h"
// Server쪽 State
#include "State.h"

CState::CState(AI_STATE _state)
	: state(_state)
	, AI(nullptr)
{
}

CState::~CState()
{
}
