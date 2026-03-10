#pragma once
#include "Character.h"

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

protected:
    void            MonsterMoveSyncByInterpolation(float elapsedTime);

protected:
    XMFLOAT3 origin_position;
    AI_STATE AI_state;
    MON_TYPE monster_type;

    //==================================================
    // 서버쪽에서 전달받은 몬스터의 정보 (서버 관련)
    float       smoothed_delay = 0.1f;
    MonsterInfo dest_info;
    std::deque<MonsterFrameHistory> interpolation_deq;
    //==================================================
};

