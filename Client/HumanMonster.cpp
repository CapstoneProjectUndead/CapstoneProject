#include "stdafx.h"
#include "HumanMonster.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"

CHumanMonster::CHumanMonster()
    : attack_timer(0.0f)
    , idle_timer(0.0f)
{
    friction = 0.0f;
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
    // 시야 범위에 플레이어가 들어오는지 체크
    auto target = FindNearestPlayer();
    if (target) {
        float dist = Vector3::Length(Vector3::Subtract(target->position, this->position));
        if (dist <= GetRecogRange()) {
            SetTarget(target);
            auto AIComponent = GetComponent<CAIComponent>();
            if (AIComponent) {
                AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
                return; // 상태가 바뀌었으니 여기서 함수 종료! (아래 배회 로직 스킵)
            }
        }
    }

    // 배회 

    // 타이머 증가
    idle_timer += elapsedTime;

    if (idle_timer >= 2.0f) {
        float newYaw = yaw + 180.0f;
        SetYaw(newYaw);            
        SetYawPitch(newYaw, 0.0f);
        idle_timer = 0.0f;
    }

    // CMovementComponent::Move 함수 대신 velocity를 직접 세팅!
    // CObject에 있는 look 벡터(현재 바라보는 방향)를 활용.
    float walk_speed = 0.4f; // 원하는 산책 속도로 조절

    velocity.x = look.x * walk_speed;
    velocity.z = look.z * walk_speed;
}

void CHumanMonster::OnTraceMove(float elapsedTime)
{
    auto AIComponent = GetComponent<CAIComponent>();

    if (!target_player) {
        // 타겟이 사라졌으면 IDLE로 복귀
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }
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
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
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