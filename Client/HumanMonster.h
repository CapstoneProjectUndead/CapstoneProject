#pragma once
#include "Monster.h"

class CPlayer;

class CHumanMonster :
    public CMonster
{
public:
    CHumanMonster();
    ~CHumanMonster();

    virtual void Update(float elapsedTime) override;
    void OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers) override;
    virtual void OnIdleMove(float elapsedTime) override;
    virtual void OnPatrolMove(float elapsedTime) override;
    virtual void OnTraceMove(float elapsedTime) override;
    virtual void OnAttackMove(float elapsedTime) override;

    virtual void OnIdleEnter() override;
    virtual void OnPatrolEnter() override;
    virtual void OnAttackEnter() override;
};

