#pragma once
#include "Monster.h"
#include "MapGenerator/MapGenerator.h"

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
    virtual void OnTraceEnter() override;
    virtual void OnAttackEnter() override;

private:
    void     PatrolRadiusWander(float elapsedTime);
    XMFLOAT3 GetRandomWanderTarget();

private:
    // Stuck 감지
    float    stuck_check_timer = 0.0f;
    XMFLOAT3 last_stuck_pos    = {};

    // RADIUS_WANDER
    XMFLOAT3    wander_target        = {};
    float       wander_wait_timer    = 0.0f;
    float       wander_wait_duration = 1.5f;

    bool        is_waiting        = false;
    const float wander_radius     = 4.0f;
    float       patrol_speed      = 1.2f;

    // ATTACK
    bool  hit_damage_dealt       = false;
    float contact_damage_timer   = 0.0f;

    // TRACE 경로 탐색
    std::vector<MapGenerator::Cell> nav_path;
    float path_refresh_timer  = 0.0f;
};

