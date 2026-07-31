#pragma once
#include "Monster.h"
#include "MapGenerator/MapGenerator.h"

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
    virtual void OnTraceEnter() override;
    virtual void OnAttackEnter() override;
    virtual void OnFleeEnter() override;
    virtual void OnFleeExit() override;

    virtual void OnFleeMove(float elapsedTime) override;

public:
    // Dog 소환
    void SetSpawnCallback(std::function<void(MON_TYPE, XMFLOAT3)> fn) { spawn_callback = std::move(fn); }

private:
    bool  hit_damage_dealt      = false;
    float attack_cooldown_timer = 9999.f;

    float dog_spawn_timer = 0.f;
    bool  has_called_dogs = false;

    float flee_timer = 0.0f;

    XMFLOAT3 store_center_world = {};
    bool     has_store_center   = false;
    void     InitStoreCenter();
    void     UpdateStoreAlert(float elapsedTime);

    // 순찰 중 상점 내부 배회/장애물 우회용
    XMFLOAT3 prev_patrol_pos = {};
    float    stuck_timer     = 0.0f;

    std::function<void(MON_TYPE, XMFLOAT3)> spawn_callback;

    void SpawnCallDogs();

    static constexpr float  DOG_SPAWN_DELAY       = 1.5f;
    static constexpr int    STORE_TRIGGER_TILES   = 1;
    // 상점 3x3(타일 2.0 x 3 = 중심 기준 ±3.0) 안쪽으로만 배회. 콜라이더 여유분을 빼고 2.4
    static constexpr float  STORE_WANDER_BOUND    = 2.4f;
    static constexpr float  FLEE_DURATION  = 1.0f;
    static constexpr float  FLEE_SPEED     = 0.5f;
};

