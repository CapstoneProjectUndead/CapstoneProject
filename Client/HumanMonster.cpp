#include "stdafx.h"
#include "HumanMonster.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "Animator.h"
#include "SoundManager.h"
#include "GameScene.h"
#include "State.h"

CHumanMonster::CHumanMonster()
    : CMonster(MON_TYPE::HUMAN_MONSTER)
{
    friction = 0.0f;
    SetFOV(120);
    respawn_time = 30.f;
}

CHumanMonster::~CHumanMonster()
{
}

void CHumanMonster::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);

	if (melee_knockback_timer > 0.0f) {
		float ratio = melee_knockback_timer / MELEE_KNOCKBACK_DURATION;
		velocity.x = melee_knockback_vel.x * ratio;
		velocity.z = melee_knockback_vel.z * ratio;

		melee_knockback_timer -= elapsedTime;

		if (melee_knockback_timer < 0.0f)
            melee_knockback_timer = 0.0f;
	}

    UpdateStoreAlert(elapsedTime);
}

void CHumanMonster::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
    CObject::OnCollect(renderers);

    auto animator = GetComponent<CAnimatorComponent>();
    if (animator) {
        animator->RenderSocketModel(CAnimatorComponent::HAND_R, NULL, "flapper");
    }
}

void CHumanMonster::UpdateStoreAlert(float elapsedTime)
{
    if (!has_store_center)
        InitStoreCenter();

    if (!has_store_center)
        return;

    if (dog_spawn_timer > 0.f) {
        dog_spawn_timer -= elapsedTime;
        if (dog_spawn_timer <= 0.f) {
            dog_spawn_timer = 0.f;
            SpawnCallDogs();
        }
    }
    else if (!has_called_dogs) {
        auto target = FindNearestPlayer();
        if (target) {
            XMFLOAT3 diff = Vector3::Subtract(store_center_world, target->position);
            diff.y = 0.f;
            float distSq = diff.x * diff.x + diff.z * diff.z;
            if (distSq <= STORE_TRIGGER_RADIUS * STORE_TRIGGER_RADIUS) {
                SetTarget(target);
                CSoundManager::GetInstance().Play(SOUND_ID::warning_bell);
                dog_spawn_timer = DOG_SPAWN_DELAY;
                if (auto ai = GetComponent<CAIComponent>())
                    ai->ChangeState(AI_STATE::MONSTER_TRACE);
            }
        }
    }
}

void CHumanMonster::InitStoreCenter()
{
    // SetOriginPos가 아직 호출되지 않은 상태라면 초기화를 미룬다.
    // (CObject::Initialize()→Update(0.f) 경로에서 origin이 (0,0)인 채로 호출되는 것을 방지)
    if (fabsf(origin_position.x) < 0.001f && fabsf(origin_position.z) < 0.001f)
        return;

    const auto& centers = MapGenerator::GetStoreCenters();
    float minDistSq = FLT_MAX;
    for (const auto& cell : centers) {
        XMFLOAT3 wp = { cell.x * 2.f, 0.f, cell.y * 2.f };
        float dx = wp.x - origin_position.x;
        float dz = wp.z - origin_position.z;
        float dSq = dx * dx + dz * dz;
        if (dSq < minDistSq) {
            minDistSq = dSq;
            store_center_world = wp;
        }
    }
    has_store_center = true;
}

void CHumanMonster::OnIdleMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f)
        return;

    if (has_called_dogs) {
        auto target = FindNearestPlayer();
        if (target) {
            XMFLOAT3 diff = Vector3::Subtract(target->position, position);
            diff.y = 0.f;
            if (diff.x * diff.x + diff.z * diff.z <= recog_range * recog_range) {
                SetTarget(target);
                if (auto ai = GetComponent<CAIComponent>())
                    ai->ChangeState(AI_STATE::MONSTER_TRACE);
                return;
            }
        }
    }

    // 초기 자리(origin_position)로 복귀하기
    XMFLOAT3 dirToOrigin = Vector3::Subtract(origin_position, position);
    dirToOrigin.y = 0.0f;
    float distToOrigin = Vector3::Length(dirToOrigin);

    if (distToOrigin > 0.1f) {

        // 플레이어가 초기 위치로 복귀하는 동안에는 걷는 애니메이션이 나와야한다.
        AI_state = AI_STATE::MONSTER_PATROL;

        const float TILE_SIZE = 2.0f;
        path_refresh_timer += elapsedTime;
        if (path_refresh_timer >= 0.3f || nav_path.empty()) {
            path_refresh_timer = 0.0f;
            int sx = (int)roundf(position.x / TILE_SIZE);
            int sz = (int)roundf(position.z / TILE_SIZE);
            int ex = (int)roundf(origin_position.x / TILE_SIZE);
            int ez = (int)roundf(origin_position.z / TILE_SIZE);
            nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
        }

        XMFLOAT3 moveDir = dirToOrigin;
        if (!nav_path.empty()) {
            XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
            XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
            toWp.y = 0.0f;
            if (Vector3::Length(toWp) > 0.1f)
                moveDir = toWp;
        }

        float returnYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
        SetYaw(returnYaw);
        SetYawPitch(returnYaw, 0.0f);

        float walk_speed = 0.5f;
        velocity.x = look.x * walk_speed;
        velocity.z = look.z * walk_speed;

        return;
    }

    // 초기 지점으로 복귀하면 
    {
        SetYaw(0.0f);
        SetYawPitch(0.0f, 0.0f);
        AI_state = AI_STATE::MONSTER_IDLE;
    }

    // 속도 0 고정 (휴식)
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 타이머 체크: 5초 쉬었으면 순찰(PATROL)하러 출발
    idle_timer += elapsedTime;

    if (idle_timer >= 5.0f) {

        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_PATROL);
    }
}

void CHumanMonster::OnPatrolMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f)
        return;

    patrol_timer += elapsedTime;
    turn_timer += elapsedTime;

    // 전체 배회 시간 (15초)
    if (patrol_timer >= 15.0f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 방향 전환 (6초)
    if (turn_timer >= 6.0f) {
        float newYaw = yaw + 180.0f;
        SetYaw(newYaw);
        SetYawPitch(newYaw, 0.0f);
        turn_timer = 0.0f;
    }

    // 이동 처리
    float walk_speed = 0.4f;
    velocity.x = look.x * walk_speed;
    velocity.z = look.z * walk_speed;
}

void CHumanMonster::OnTraceMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f) 
        return;

    auto AIComponent = GetComponent<CAIComponent>();

    auto targetPlayer = target_player.lock();

    if (!targetPlayer) {
        // 타겟이 사라졌으면 IDLE로 복귀
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 타겟과의 방향 및 평면 거리 계산
    XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->position, position);
    dirVec.y = 0.0f; // Y축(높이) 차이는 무시하고 XZ 평면에서의 거리만 계산
    float dist = Vector3::Length(dirVec);

    attack_cooldown_timer += elapsedTime;

    if (dist > recog_range) {
        target_player.reset();
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }
    else if (dist <= attack_range && attack_cooldown_timer >= 0.4f) {
        AIComponent->ChangeState(AI_STATE::MONSTER_ATTACK);
        return;
    }
    else if (dist <= attack_range) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        AI_state = AI_STATE::MONSTER_IDLE;
        return;
    }

    AI_state = AI_STATE::MONSTER_TRACE;

    const float TILE_SIZE = 2.0f;
    int sx = (int)roundf(position.x / TILE_SIZE);
    int sz = (int)roundf(position.z / TILE_SIZE);
    int ex = (int)roundf(targetPlayer->position.x / TILE_SIZE);
    int ez = (int)roundf(targetPlayer->position.z / TILE_SIZE);

    // Bresenham 직선으로 벽 여부 확인 — 막힘 없으면 직진, 막히면 BFS
    bool hasWall = false;
    {
        int x = sx, z = sz;
        int dx = abs(ex - sx), dz = abs(ez - sz);
        int stepX = (ex > sx) ? 1 : -1;
        int stepZ = (ez > sz) ? 1 : -1;
        int err = dx - dz;

        while (x != ex || z != ez) {
            if (MapGenerator::IsBlockedStructure(x, z)) {
                hasWall = true;
                break;
            }
            int e2 = 2 * err;
            if (e2 > -dz) { err -= dz; x += stepX; }
            if (e2 <  dx) { err += dx; z += stepZ; }
        }
    }

    XMFLOAT3 moveDir = dirVec;
    if (hasWall) {
        path_refresh_timer += elapsedTime;
        if (path_refresh_timer >= 0.2f || nav_path.empty()) {
            path_refresh_timer = 0.0f;
            nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
        }
        if (!nav_path.empty()) {
            XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
            XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
            toWp.y = 0.0f;
            if (Vector3::Length(toWp) > 0.1f)
                moveDir = toWp;
        }
    }
    else {
        nav_path.clear();
        path_refresh_timer = 0.0f;
    }

    float targetYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
    SetYaw(targetYaw);
    SetYawPitch(targetYaw, 0.0f);

    velocity.x = look.x * trace_speed;
    velocity.z = look.z * trace_speed;
}

void CHumanMonster::OnAttackMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f)
        return;

    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 같은 자리에 박힌 다른 Human과 분리
    ApplySeparation(0.5f);

    attack_timer += elapsedTime;

    if (!hit_damage_dealt && attack_timer >= 0.45f) {

        CSoundManager::GetInstance().Play(SOUND_ID::jab);

        hit_damage_dealt = true;

        auto targetPlayer = target_player.lock();
        if (targetPlayer && targetPlayer->GetIsMyPlayer()) {
            XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->position, position);
            dirVec.y = 0.0f;

            XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
            XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

            float forwardDist = Vector3::DotProduct(dirVec, fwd);
            float sideDist    = Vector3::DotProduct(dirVec, right_vec);

            constexpr float depth = 1.3f;  // 조절 가능
            constexpr float width = 0.8f;  // 조절 가능

            if (forwardDist >= -0.3f && forwardDist <= depth && fabsf(sideDist) <= width) {
                CSoundManager::GetInstance().Play(SOUND_ID::damaged1);
                uint32 hp = targetPlayer->GetHp();
                targetPlayer->SetHp(hp > 10 ? hp - 10 : 0);
                XMFLOAT3 knockbackDir = Vector3::Subtract(targetPlayer->position, position);
                static_cast<CMyPlayer*>(targetPlayer.get())->ApplyKnockback(knockbackDir, 0.6f, 0.f);
            }
        }
    }

    if (attack_timer >= 1.5f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
    }
}

void CHumanMonster::OnFleeEnter()
{
    CSoundManager::GetInstance().Play(SOUND_ID::girl_flee);

    flee_timer = FLEE_DURATION;
    nav_path.clear();
    path_refresh_timer = 0.0f;
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    auto targetPlayer = target_player.lock();
    if (!targetPlayer) {
        auto nearest = FindNearestPlayer();
        if (nearest)
            SetTarget(nearest);
        targetPlayer = target_player.lock();
    }

    if (targetPlayer) {
        XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->position);
        awayDir.y = 0.0f;
        float len = Vector3::Length(awayDir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }

    DropItem();
}

void CHumanMonster::OnFleeMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f) return;

    flee_timer -= elapsedTime;
    if (flee_timer <= 0.0f) {
        MarkForDelete();
        return;
    }

    auto targetPlayer = target_player.lock();
    if (targetPlayer) {
        XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->position);
        awayDir.y = 0.0f;
        float len = Vector3::Length(awayDir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }

    velocity.x = look.x * FLEE_SPEED;
    velocity.z = look.z * FLEE_SPEED;
}

void CHumanMonster::OnFleeExit()
{
    flee_timer = 0.0f;
    velocity.x = 0.0f;
    velocity.z = 0.0f;
}

void CHumanMonster::SpawnCallDogs()
{
    has_called_dogs = true;
    if (!spawn_callback) 
        return;

    for (int i = 0; i < 2; i++) {
        float offset = (i == 0) ? 1.f : -1.f;
        spawn_callback(MON_TYPE::ANIMAL_MONSTER, { position.x + offset, 0.1f, position.z });
    }
}

void CHumanMonster::OnIdleEnter()
{
    ResetIdleTimer();
    nav_path.clear();
    path_refresh_timer = 0.0f;
}

void CHumanMonster::OnPatrolEnter()
{
    ResetPatrolTimers();
}

void CHumanMonster::OnTraceEnter()
{
    nav_path.clear();
    path_refresh_timer = 0.0f;
}

void CHumanMonster::OnAttackEnter()
{
    ResetAttackTimer();

    velocity.x            = 0.0f;
    velocity.z            = 0.0f;
    attack_timer          = 0.0f;
    hit_damage_dealt      = false;
    attack_cooldown_timer = 0.0f;

    auto targetPlayer = target_player.lock();
    if (targetPlayer) {
        XMFLOAT3 dir = Vector3::Subtract(targetPlayer->position, position);
        dir.y = 0.0f;
        float len = Vector3::Length(dir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }
}