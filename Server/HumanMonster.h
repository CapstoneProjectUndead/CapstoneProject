#pragma once
// Server쪽 HumanMonster
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
    virtual void OnPatrolMove(float elapsedTime) override;
    virtual void OnTraceMove(float elapsedTime) override;
    virtual void OnAttackMove(float elapsedTime) override;

    virtual void OnIdleEnter() override;
    virtual void OnPatrolEnter() override;
    virtual void OnAttackEnter() override;

public:
    void SetOriginPos(const XMFLOAT3& pos) { origin_position = pos; }
    const XMFLOAT3& GetOriginPos() const { return origin_position; }

private:
    shared_ptr<CPlayer> FindNearestPlayer();

    void SetTarget(shared_ptr<CPlayer> player) { target_player = player; }

    void ResetPatrolTimers() 
    {
        patrol_timer = 0.0f;
        turn_timer = 0.0f;
    }

    void ResetIdleTimer() { idle_timer = 0.0f; }
    void ResetAttackTimer() { attack_timer = 0.0f; }

private:
    XMFLOAT3 origin_position;
    shared_ptr<CPlayer> target_player;

    float idle_timer;   // 쉴 때 쓰는 타이머
    float patrol_timer; // 순찰할 때 쓰는 타이머
    float attack_timer; // 공격 상태에서 시간을 잴 타이머
    float turn_timer;

    const float recog_range = 1.f;
    const float attack_range = 0.5f;
    const float trace_speed = 0.6f;
};

