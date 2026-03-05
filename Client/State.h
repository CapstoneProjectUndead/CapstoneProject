#pragma once

class CObject;
class CAIComponent;

class CState
{
	friend class CAIComponent;
public:
	CState(MON_STATE _state);
	~CState();

	virtual void Update(float elapsedTime) = 0;
	virtual void Enter() = 0;
	virtual void Exit() = 0;

public:
	CAIComponent* GetAI() { return AI; }
	MON_STATE     GetType() { return state; }

protected:
	CAIComponent* AI;
	MON_STATE	  state;
};

