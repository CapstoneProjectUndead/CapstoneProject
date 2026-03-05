#pragma once
#include "Monster.h"

class CPlayer;

class CHumanMonster :
    public CMonster
{
public:
    CHumanMonster();
    ~CHumanMonster();

    virtual void Update(float elapsedTime) override;
    virtual void OnIdleMove(float elapsedTime) override;
    virtual void OnTraceMove(float elapsedTime) override;
    virtual void OnAttackMove(float elapsedTime) override;

    virtual void OnAttackEnter() override;

private:
    std::shared_ptr<CPlayer> FindNearestPlayer();

    // 몬스터 스펙 (필요시 멤버 변수로 빼서 기획 데이터로 로드해도 됩니다)
    float GetRecogRange() const { return 10.0f; }  // 인식 거리 10m
    float GetAttackRange() const { return 2.0f; }  // 공격 사거리 2m
    float GetMoveSpeed() const { return 4.0f; }    // 추적 이동 속도

    void SetTarget(std::shared_ptr<CPlayer> player) { target_player = player; }

private:
    std::shared_ptr<CPlayer> target_player;
    float attack_timer; // 공격 상태에서 시간을 잴 타이머
};

