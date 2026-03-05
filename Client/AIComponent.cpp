#include "stdafx.h"
#include "AIComponent.h"
#include "State.h"

CAIComponent::CAIComponent()
	: current_state(nullptr)
{
}

CAIComponent::~CAIComponent()
{
}

void CAIComponent::Update(float elapsedTime)
{
	auto iter = states.find(current_state->GetType());
	assert(iter->second);
	iter->second->Update(elapsedTime);
}

void CAIComponent::AddState(std::shared_ptr<CState> state)
{
	std::shared_ptr<CState> findState = GetState(state->GetType());
	assert(!findState);

	states.insert(make_pair(state->GetType(), state));
	state->AI = this;
}

std::shared_ptr<CState> CAIComponent::GetState(MON_STATE state)
{
	StateMap::iterator iter = states.find(state);
	if (iter == states.end())
		return nullptr;

	return iter->second;
}

void CAIComponent::SetState(MON_STATE state)
{
	current_state = GetState(state);
	assert(current_state);
}

void CAIComponent::ChangeState(MON_STATE _nextState)
{
	std::shared_ptr<CState> nextState = GetState(_nextState);
	assert(current_state != nextState);
	current_state->Exit();
	current_state = nextState;
	current_state->Enter();
}
