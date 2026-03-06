#pragma once
#include "State.h"

class CPatrolState :
    public CState
{
public:
    CPatrolState();
    ~CPatrolState();

    virtual void Update(float elapsedTime) override;
    virtual void Enter() override;
    virtual void Exit() override;
};

