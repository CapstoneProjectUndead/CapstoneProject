#pragma once
// Server쪽 Monster
#include "Object.h"
#include <MapGenerator/MapGenerator.h>

class CScene;
class CPlayer;
class CAIComponent;

class CMonster :
    public CObject
{
public:
    CMonster(MON_TYPE type);
    ~CMonster();

    virtual void Update(float elapsedTime) override;

    // AI 순수 가상 함수 (반드시 구현)
    virtual void OnIdleMove(float elapsedTime) = 0;
    virtual void OnPatrolMove(float elapsedTime) = 0;
    virtual void OnTraceMove(float elapsedTime) = 0;
    virtual void OnAttackMove(float elapsedTime) = 0;

    // AI Enter (진입 시 - 필요한 애들만 오버라이딩하게)
    virtual void OnIdleEnter() {}
    virtual void OnPatrolEnter() {}
    virtual void OnTraceEnter() {}
    virtual void OnAttackEnter() {}
    virtual void OnFleeEnter() {}

    // AI Exit (탈출 시 - 필요한 애들만 오버라이딩하게)
    virtual void OnIdleExit() {}
    virtual void OnPatrolExit() {}
    virtual void OnTraceExit() {}
    virtual void OnAttackExit() {}
    virtual void OnFleeExit() {}

    // AI Move (필요한 애들만 오버라이딩하게)
    virtual void OnFleeMove(float elapsedTime) {}

public:
    AI_STATE GetAIState() const { return AI_state; }
    void     SetAIState(AI_STATE state) { AI_state = state; }

    MON_TYPE GetMonsterType() const { return monster_type; }

    float GetRespawnTime() const { return respawn_time; }

    void SetOriginPos(const XMFLOAT3& pos) { origin_position = pos; }
    const XMFLOAT3& GetOriginPos() const { return origin_position; }

    virtual void ApplyMeleeHit(const XMFLOAT3& fromPos, shared_ptr<CPlayer> player);

    CScene* GetScene() const { return current_scene; }
    void    SetScene(CScene* scene) { current_scene = scene; }

protected:
    std::shared_ptr<CPlayer> FindNearestPlayer();

    // 같은 종 몬스터끼리 겹침 방지 (Separation Steering)
    void ApplySeparation(float scale = 1.0f);

    void SetTarget(std::shared_ptr<CPlayer> player) { target_player = player; }

    void ResetPatrolTimers()
    {
        patrol_timer = 0.0f;
        turn_timer = 0.0f;
    }

    void ResetIdleTimer() { idle_timer = 0.0f; }
    void ResetAttackTimer() { attack_timer = 0.0f; }

    void SetFOV(float angle)
    {
        fov_angle = angle;
        cos_threshold = cosf(XMConvertToRadians(fov_angle * 0.5f));
    }

    void DropItem(uint16 itemID);

protected:
    XMFLOAT3 origin_position;
    AI_STATE AI_state;
    MON_TYPE monster_type;
    float    respawn_time;
    CScene* current_scene;

    std::weak_ptr<CPlayer> target_player;

    float idle_timer;   // 쉴 때 쓰는 타이머
    float patrol_timer; // 순찰할 때 쓰는 타이머
    float attack_timer; // 공격 상태에서 시간을 잴 타이머
    float turn_timer;

    float recog_range = 5.0f; // 인지 범위
    float fov_angle = 120.f; // 시야각
    float cos_threshold; // 미리 계산된 코사인 임계값

    float attack_range = 1.2f; // 공격 범위
    float trace_speed = 2.0f;  // 추격 속도

    // 근접 피격 넉백
    float     melee_knockback_timer = 0.0f;
    XMFLOAT3  melee_knockback_vel   = {};
    static constexpr float MELEE_KNOCKBACK_DURATION = 0.25f;
    static constexpr float MELEE_KNOCKBACK_FORCE    = 3.0f;

    // BFS 경로 탐색 (TRACE 추적 / IDLE 복귀 공용)
    std::vector<MapGenerator::Cell> nav_path;
    float path_refresh_timer = 0.0f;
};

