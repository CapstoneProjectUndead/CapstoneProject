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

    // AI Exit (탈출 시 - 필요한 애들만 오버라이딩하게)
    virtual void OnIdleExit() {}
    virtual void OnPatrolExit() {}
    virtual void OnTraceExit() {}
    virtual void OnAttackExit() {}

public:
    AI_STATE GetAIState() const { return AI_state; }
    void     SetAIState(AI_STATE state) { AI_state = state; }

    MON_TYPE GetMonsterType() const { return monster_type; }

    void SetOriginPos(const XMFLOAT3& pos) { origin_position = pos; }
    const XMFLOAT3& GetOriginPos() const { return origin_position; }

    CScene* GetScene() const { return current_scene; }
    void    SetScene(CScene* scene) { current_scene = scene; }

protected:
    std::shared_ptr<CPlayer> FindNearestPlayer();

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

protected:
    XMFLOAT3 origin_position;
    AI_STATE AI_state;
    MON_TYPE monster_type;
    CScene* current_scene;

    std::weak_ptr<CPlayer> target_player;

    float idle_timer;   // 쉴 때 쓰는 타이머
    float patrol_timer; // 순찰할 때 쓰는 타이머
    float attack_timer; // 공격 상태에서 시간을 잴 타이머
    float turn_timer;

    float recog_range = 3.0f; // 인지 범위
    float fov_angle = 120.f; // 시야각
    float cos_threshold; // 미리 계산된 코사인 임계값

    float attack_range = 1.2f; // 공격 범위
    float trace_speed = 2.0f;  // 추격 속도

    // BFS 경로 탐색 (TRACE 추적 / IDLE 복귀 공용)
    std::vector<MapGenerator::Cell> nav_path;
    float path_refresh_timer = 0.0f;
};

