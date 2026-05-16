#pragma once
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
    virtual void OnFleeMove(float elapsedTime) override;

    virtual void OnIdleEnter() override;
    virtual void OnPatrolEnter() override;
    virtual void OnTraceEnter() override;
    virtual void OnAttackEnter() override;
    virtual void OnFleeEnter() override;

    virtual void OnFleeExit() override;

public:
    void ApplySprayHit(const XMFLOAT3& fromPos);

private:
    void CheckContactDamage();

private:
    // ATTACK
    bool  hit_damage_dealt      = false;
    bool  stun_applied          = false;
    float contact_damage_timer  = 0.0f;
    float attack_cooldown_timer = 9999.f;

    // FLEE
    float flee_timer = 0.0f;
    static constexpr float  FLEE_DURATION = 1.0f;
    static constexpr float  FLEE_SPEED    = 1.0f;

    // 스프레이 피격
    int       spray_hit_count   = 0;
    float     knockback_timer   = 0.0f;
    XMFLOAT3  knockback_vel     = {};

    static constexpr int   MAX_SPRAY_HITS     = 3;
    static constexpr float KNOCKBACK_DURATION = 0.3f;
    static constexpr float KNOCKBACK_FORCE    = 3.0f;

};

