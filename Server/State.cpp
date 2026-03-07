#include "pch.h"
// ServerÂÊ State
#include "State.h"

CState::CState(AI_STATE _state)
	: state(_state)
	, AI(nullptr)
{
}

CState::~CState()
{
}
