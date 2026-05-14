#pragma once
// Server쪽 HumanMonster
#include "Monster.h"

class CPlayer;

class CHumanMonster :
    public CMonster
{
public:
    CHumanMonster();
    ~CHumanMonster();

    virtual void Update(float elapsedTime) override;
    virtual void OnIdleMove(float elapsedTime) override;
    virtual void OnPatrolMove(float elapsedTime) override;
    virtual void OnTraceMove(float elapsedTime) override;
    virtual void OnAttackMove(float elapsedTime) override;

    virtual void OnIdleEnter() override;
    virtual void OnPatrolEnter() override;
    virtual void OnTraceEnter() override;
    virtual void OnAttackEnter() override;
    virtual void OnFleeEnter() override;
    virtual void OnFleeExit() override;

    virtual void ApplyMeleeHit(const XMFLOAT3& fromPos, shared_ptr<CPlayer> player) override;
    virtual void OnFleeMove(float elapsedTime) override;

private:
    bool  hit_damage_dealt      = false;
    float attack_cooldown_timer = 9999.f;

    int   melee_hit_count = 0;
    float flee_timer      = 0.0f;

    static constexpr int    MAX_MELEE_HITS = 5;
    static constexpr float  FLEE_DURATION  = 1.0f;
    static constexpr float  FLEE_SPEED     = 0.5f;
};

