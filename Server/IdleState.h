#pragma once
// Server쪽 IdleState
#include "State.h"

class CIdleState :
    public CState
{
public:
    CIdleState();
    ~CIdleState();

    virtual void Update(float elapsedTime) override;
    virtual void Enter() override;
    virtual void Exit() override;
};

