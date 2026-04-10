#pragma once
// Server쪽 State
#include "Component.h"

class CState;

class CAIComponent :
    public CComponent
{
	using StateMap = std::map<AI_STATE, std::shared_ptr<CState>>;
public:
	CAIComponent();
	~CAIComponent();

	virtual void Update(const float deltaTime) override;

public:
	void					AddState(std::shared_ptr<CState> state);

	std::shared_ptr<CState> GetState(AI_STATE state);
	void					SetState(AI_STATE state);

	std::shared_ptr<CState> GetCurrentState() const { return current_state; }

	void					ChangeState(AI_STATE _nextState);

private:
	std::map<AI_STATE, std::shared_ptr<CState>>	states;
	std::shared_ptr<CState>						current_state;
};

