#pragma once
// Server쪽 DogMonster
#include "Monster.h"

class CPlayer;

class CDogMonster :
    public CMonster
{
public:
    CDogMonster();
    ~CDogMonster();

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
    virtual void OnFleeMove(float elapsedTime) override;
    virtual void OnFleeExit() override;

    virtual void ApplyMeleeHit(const XMFLOAT3& fromPos, shared_ptr<CPlayer> player) override;

private:
    bool  hit_damage_dealt      = false;
    float attack_cooldown_timer = 9999.f;
    float path_fail_timer       = 0.0f;
    float give_up_cooldown      = 0.0f;

    int   melee_hit_count = 0;
    float flee_timer      = 0.0f;

    static constexpr float IDLE_DURATION      = 4.0f;
    static constexpr float PATROL_DURATION    = 5.0f;
    static constexpr float TURN_INTERVAL      = 2.0f;
    static constexpr float PATROL_SPEED       = 4.0f;
    static constexpr float ATTACK_HIT_TIME    = 0.7f;
    static constexpr float ATTACK_DURATION    = 1.0f;
    static constexpr float CHASE_GIVE_UP_TIME = 1.5f;
    static constexpr float GIVE_UP_COOLDOWN   = 4.0f;
    static constexpr int   MAX_MELEE_HITS     = 5;
    static constexpr float FLEE_DURATION      = 1.0f;
    static constexpr float FLEE_SPEED         = 0.5f;

    // 개 짖는 소리
    bool dog_bark = false;
    float dog_bark_time = 0.f;
};
