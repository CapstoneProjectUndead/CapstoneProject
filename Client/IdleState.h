#pragma once
#include "State.h"

class CIdleState :
    public CState
{
public:
    CIdleState();
    ~CIdleState();

    virtual void Update() override;
    virtual void Enter() override;
    virtual void Exit() override;
};

