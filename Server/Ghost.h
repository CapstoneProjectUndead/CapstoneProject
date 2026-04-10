#pragma once
// Server쪽 Ghost
#include "Monster.h"

class CGhost :
    public CMonster
{
public:
    CGhost();
    ~CGhost();

    virtual void Update(float elapsedTime) override;
    virtual void OnIdleMove(float elapsedTime) override;
    virtual void OnPatrolMove(float elapsedTime) override;
    virtual void OnTraceMove(float elapsedTime) override;
    virtual void OnAttackMove(float elapsedTime) override;

    virtual void OnIdleEnter() override;
    virtual void OnPatrolEnter() override;
    virtual void OnAttackEnter() override;
};

