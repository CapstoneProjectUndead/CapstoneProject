#pragma once

class CAIComponent;

class CState
{
	friend class CAIComponent;
public:
	CState(MON_STATE _state);
	~CState();

	virtual void Update() = 0;
	virtual void Enter() = 0;
	virtual void Exit() = 0;

public:
	CAIComponent* GetAI() { return AI; }
	MON_STATE     GetType() { return state; }

private:
	CAIComponent* AI;
	MON_STATE	  state;
};

