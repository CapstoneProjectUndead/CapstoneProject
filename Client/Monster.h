#pragma once
#include "Character.h"
#include "MapGenerator/MapGenerator.h"

struct MonsterFrameHistory
{
    uint32	 monster_id;
    float	 server_timestamp; // 서버에서 찍어준 도장
    XMFLOAT3 position;
    float    yaw;
    float    pitch;
    AI_STATE AI_state;
    MON_TYPE type;
};

class CPlayer;
class CAIComponent;

class CMonster :
    public CCharacter
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
    virtual void OnFleeMove(float elapsedTime) {}

    // AI Enter (진입 시 - 필요한 애들만 오버라이딩하게)
    virtual void OnIdleEnter() {}
    virtual void OnPatrolEnter() {}
    virtual void OnTraceEnter() {}
    virtual void OnAttackEnter() {}
    virtual void OnFleeEnter(){}

    // AI Exit (탈출 시 - 필요한 애들만 오버라이딩하게)
    virtual void OnIdleExit() {}
    virtual void OnPatrolExit() {}
    virtual void OnTraceExit() {}
    virtual void OnAttackExit() {}
    virtual void OnFleeExit() {}

private:
    void UpdateSingle(float elapsedTime);
    void UpdateMulti(float elapsedTime);

public:
    AI_STATE GetAIState() const { return AI_state; }
    void     SetAIState(AI_STATE state) { AI_state = state; }

    MON_TYPE GetMonsterType() const { return monster_type; }

    void            SetOriginPos(const XMFLOAT3& pos) { origin_position = pos; }
    const XMFLOAT3& GetOriginPos() const { return origin_position; }

    void            SetDestInfo(const MonsterInfo& pos) { dest_info = pos; }
    void            RecordMonsterFrameHistory(const MonsterFrameHistory& state);

    virtual void ApplyMeleeHit(const XMFLOAT3& fromPos);

protected:
    void MonsterMoveSyncByInterpolation(float elapsedTime);

    std::shared_ptr<CPlayer> FindNearestPlayer();

    void SetTarget(std::shared_ptr<CPlayer> player) { target_player = player; }

    void ResetPatrolTimers()
    {
        patrol_timer = 0.0f;
        turn_timer = 0.0f;
    }

    void ResetIdleTimer() { idle_timer = 0.0f; }
    void ResetAttackTimer() { attack_timer = 0.0f; }

    // 몬스터 시야각 추가
    void SetFOV(float angle) 
    {
        fov_angle = angle;
        cos_threshold = cosf(XMConvertToRadians(fov_angle * 0.5f));
    }

protected:
    XMFLOAT3 origin_position;
    AI_STATE AI_state;
    MON_TYPE monster_type;

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

    // 근접 피격 넉백
    float     melee_knockback_timer = 0.0f;
    XMFLOAT3  melee_knockback_vel   = {};
    static constexpr float MELEE_KNOCKBACK_DURATION = 0.25f;
    static constexpr float MELEE_KNOCKBACK_FORCE    = 3.0f;

    //==================================================
    // 서버쪽에서 전달받은 몬스터의 정보 (서버 관련)
    float       smoothed_delay = 0.1f;
    MonsterInfo dest_info;
    std::deque<MonsterFrameHistory> interpolation_deq;
    //==================================================

    // BFS 경로 탐색 (TRACE 추적 / IDLE 복귀 공용)
    std::vector<MapGenerator::Cell> nav_path;
    float path_refresh_timer = 0.0f;
};

