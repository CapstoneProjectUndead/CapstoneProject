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

    // Dog는 빙의된 플레이어도 추격/공격
    virtual bool CanTargetPossessed() const override { return true; }

private:
    bool  hit_damage_dealt      = false;
    float attack_cooldown_timer = 9999.f;
    float path_fail_timer       = 0.0f;
    float give_up_cooldown      = 0.0f;

    // 추격 중 콜라이더에 막혀 전진 못 하는 상태 감지용
    float    trace_stuck_timer = 0.0f;
    XMFLOAT3 trace_last_pos{};

    float flee_timer = 0.0f;

    static constexpr float IDLE_DURATION      = 4.0f;
    static constexpr float PATROL_SPEED       = 4.0f;  // origin 복귀 속도
    static constexpr float ATTACK_HIT_TIME    = 0.7f;
    static constexpr float ATTACK_DURATION    = 1.0f;
    static constexpr float CHASE_GIVE_UP_TIME = 1.5f;
    static constexpr float GIVE_UP_COOLDOWN   = 4.0f;
    static constexpr float FLEE_DURATION      = 1.0f;
    static constexpr float FLEE_SPEED         = 0.5f;

    static constexpr float STUCK_CHECK_INTERVAL = 0.5f;  // 정체 판정 주기
    static constexpr float STUCK_MOVE_EPSILON   = 0.15f; // 이 거리 미만 이동 시 정체로 판정
    static constexpr float STUCK_LUNGE_RANGE    = 1.3f;  // 정체 시 이 거리 이내면 달려들어 공격
    static constexpr float CHASE_KEEP_RANGE     = 8.0f;  // 추격 유지 거리 (감지 recog_range=5.0보다 크게 — 경계 진동 방지)

    // 개 짖는 소리
    bool dog_bark = false;
    float dog_bark_time = 0.f;
};
