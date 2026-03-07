#pragma once
#include "Character.h"

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

public:
    AI_STATE GetAIState() const { return AI_state; }
    void     SetAIState(AI_STATE state) { AI_state = state; }

    MON_TYPE GetMonsterType() const { return monster_type; }

protected:
    AI_STATE AI_state;
    MON_TYPE monster_type;
};

