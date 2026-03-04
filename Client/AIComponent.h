#pragma once
#include "Component.h"

class CState;

class CAIComponent :
    public CComponent
{
	using StateMap = std::map<MON_STATE, std::shared_ptr<CState>>;
public:
	CAIComponent();
	~CAIComponent();

	void Update();

public:
	void					AddState(std::shared_ptr<CState> state);

	std::shared_ptr<CState> GetState(MON_STATE state);
	void					SetState(MON_STATE state);

	void					ChangeState(MON_STATE _nextState);

private:
	std::map<MON_STATE, std::shared_ptr<CState>>	states;
	std::shared_ptr<CState>							current_state;
};

