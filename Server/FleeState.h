#pragma once
// Server쪽 FleeState
#include "State.h"

class CFleeState :
    public CState
{
public:
    CFleeState();
    ~CFleeState();

    virtual void Update(float elapsedTime) override;
    virtual void Enter() override;
    virtual void Exit() override;
};
