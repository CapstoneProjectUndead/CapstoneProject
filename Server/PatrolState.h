#pragma once
// Server쪽 PatrolState
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

