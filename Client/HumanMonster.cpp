#include "stdafx.h"
#include "HumanMonster.h"
#include "Player.h"
#include "AIComponent.h"

CHumanMonster::CHumanMonster()
    : attack_timer(0.0f) // 타이머 초기화
{
}

CHumanMonster::~CHumanMonster()
{
}

void CHumanMonster::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);
}

void CHumanMonster::OnIdleMove(float elapsedTime)
{
    // 테스트 단계이므로 이동은 하지 않고 부동자세 유지
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    auto target = FindNearestPlayer();

    if (target) {
        // 내 위치와 타겟 위치의 거리 계산
        float dist = Vector3::Length(Vector3::Subtract(target->position, this->position));

        // 인식 범위 안에 들어왔다면 추적 시작!
        if (dist <= GetRecogRange()) {
            SetTarget(target);
            auto AIComponent = GetComponent<CAIComponent>();
            if (AIComponent) {
                AIComponent->ChangeState(MON_STATE::TRACE);
            }
        }
    }
}

void CHumanMonster::OnTraceMove(float elapsedTime)
{
    auto AIComponent = GetComponent<CAIComponent>();

    if (!target_player) {
        // 타겟이 사라졌으면 IDLE로 복귀
        AIComponent->ChangeState(MON_STATE::IDLE);
        return;
    }

    // 오타 수정됨! (내 위치와 타겟 위치의 거리 계산)
    float dist = Vector3::Length(Vector3::Subtract(target_player->position, this->position));

    // 1. 공격 사거리 이내로 들어왔는가? -> 공격!
    if (dist <= GetAttackRange()) {
        AIComponent->ChangeState(MON_STATE::ATTACK);
        return;
    }

    // 2. 너무 멀어져서 추적을 포기해야 하는가? -> 어그로 풀림
    if (dist > GetRecogRange() * 1.5f) {
        SetTarget(nullptr);
        AIComponent->ChangeState(MON_STATE::IDLE);
        return;
    }

    // 3. 아직 멀다면 타겟을 향해 이동 (velocity 직접 세팅)
    XMFLOAT3 dir = Vector3::Normalize(Vector3::Subtract(target_player->position, this->position));

    // Y축(하늘/땅) 방향으로는 걷지 않도록 평면화
    dir.y = 0.0f;
    dir = Vector3::Normalize(dir);

    // 몬스터의 속도 세팅 (이 속도를 바탕으로 CMovementComponent가 물리 이동을 수행함)
    velocity.x = dir.x * GetMoveSpeed();
    velocity.z = dir.z * GetMoveSpeed();

    // 몬스터가 이동 방향을 쳐다보게 회전 (필요시)
    yaw = atan2f(dir.x, dir.z);
}

void CHumanMonster::OnAttackMove(float elapsedTime)
{
    // 공격 중에는 미끄러지지 않게 이동 속도 0으로 고정
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 타이머 증가
    attack_timer += elapsedTime;

    // 공격 애니메이션 길이 or 쿨타임 (예: 1.5초)이 지나면?
    if (attack_timer >= 1.5f) {
        attack_timer = 0.0f; // 타이머 초기화

        // 실제 데미지 판정 로직은 서버의 이 시점(또는 타이머 중간)에 수행!
        // 예: target_player->TakeDamage(10);

        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent) {
            // 공격이 끝났으니 다시 거리를 재기 위해 TRACE 상태로 전환
            // (TRACE 상태에서 거리가 가까우면 다음 프레임에 다시 ATTACK으로 돌아옴)
            AIComponent->ChangeState(MON_STATE::TRACE);
        }
    }
}

void CHumanMonster::OnAttackEnter()
{
    // 1. 공격 상태 진입 시, 어떤 공격을 할지 결정 (발 구르기 vs 파리채)
    // 예: int pattern = rand() % 2; 
    // DecideAttackPattern(); 

    // 2. 이동 속도를 0으로 만들어서 공격 중에 미끄러지지 않게 고정
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 3. 공격 타이머 초기화 (이제 CHumanMonster의 멤버 변수이므로 직접 접근 가능!)
    attack_timer = 0.0f;
}

std::shared_ptr<CPlayer> CHumanMonster::FindNearestPlayer()
{

	return std::shared_ptr<CPlayer>();
}