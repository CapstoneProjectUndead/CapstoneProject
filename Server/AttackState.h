#pragma once
// Server쪽 AttackState
#include "State.h"

class CAttackState :
    public CState
{
public:
    CAttackState();
    ~CAttackState();

    virtual void Update(float elapsedTime) override;
    virtual void Enter() override;
    virtual void Exit() override;
};

