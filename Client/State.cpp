#include "stdafx.h"
#include "State.h"

CState::CState(AI_STATE _state)
	: state(_state)
	, AI(nullptr)
{
}

CState::~CState()
{
}
