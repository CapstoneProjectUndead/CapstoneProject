#include "stdafx.h"
#include "Ghost.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "AnimationManager.h"
#include "Collider.h"
#include "SoundManager.h"
#include "GameScene.h"
#include "State.h"

CGhost::CGhost()
    : CMonster(MON_TYPE::GHOST)
{
    friction = 0.0f;
    trace_speed = 4.0f;
    attack_range = 1.5f;
    SetFOV(120);
    respawn_time = 90.f;
}

CGhost::~CGhost()
{
}

void CGhost::Update(float elapsedTime)
{
    // 애니메이션 프레임 수 체크용 (지우지 말 것!)
    //static bool printed = false;
    //if (!printed) {
    //    auto& clip = CAnimationManager::GetInstance().GetClip("Ghost_attack");
    //    std::cout << "Ghost_attack total_frames: " << clip.total_frames << std::endl;
    //    printed = true;
    //} 

    CMonster::Update(elapsedTime);

    contact_damage_timer += elapsedTime;

    CheckContactDamage();

    // AI가 velocity를 설정한 뒤 덮어써야 다음 프레임에 넉백이 적용된다
    if (knockback_timer > 0.0f) {
        float ratio = knockback_timer / KNOCKBACK_DURATION;
        velocity.x = knockback_vel.x * ratio;
        velocity.z = knockback_vel.z * ratio;
        knockback_timer -= elapsedTime;
        if (knockback_timer < 0.0f)
            knockback_timer = 0.0f;
    }
}

void CGhost::OnIdleMove(float elapsedTime)
{
    if (knockback_timer > 0.0f) 
        return;

     // 시야 범위에 플레이어가 들어오는지 체크
    auto target = FindNearestPlayer();

    if (target) {

        // 타겟을 향하는 방향 벡터 및 평면(XZ) 거리 계산
        XMFLOAT3 dirVec = Vector3::Subtract(target->position, position);
        dirVec.y = 0.0f; // Y축(높이) 차이는 무시
        float dist = Vector3::Length(dirVec);

        // 인식 거리(recog_range) 내에 있는지 1차 확인
        if (dist <= recog_range && dist > 0.001f) {

            // 타겟 방향 벡터 정규화 (길이를 1로 만듦)
            XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };

            // 내적(Dot Product) 계산 (Object의 look 벡터 활용)
            float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

            // 플레이어가 시야각 안에 들어왔다면 추적 시작
            if (dotProduct >= cos_threshold) {
                SetTarget(target);

                auto AIComponent = GetComponent<CAIComponent>();
                if (AIComponent) {
                    AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
                    return;
                }
            }
        }
    }

    // 초기 자리(origin_position)로 복귀하기
    XMFLOAT3 dirToOrigin = Vector3::Subtract(origin_position, position);
    dirToOrigin.y = 0.0f;
    float distToOrigin = Vector3::Length(dirToOrigin);

    if (distToOrigin > 0.1f) {

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

        float walkSpeed = 0.5f;
        velocity.x = look.x * walkSpeed;
        velocity.z = look.z * walkSpeed;

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

void CGhost::OnPatrolMove(float elapsedTime)
{
    if (knockback_timer > 0.0f) 
        return;

    // 플레이어 감지 (TRACE 전환)
    auto target = FindNearestPlayer();
    if (target) {
        XMFLOAT3 dirVec = Vector3::Subtract(target->position, position);
        dirVec.y = 0.0f;
        float dist = Vector3::Length(dirVec);

        if (dist <= recog_range && dist > 0.001f) {
            XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };
            float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

            if (dotProduct >= cos_threshold) {
                SetTarget(target);
                auto AIComponent = GetComponent<CAIComponent>();
                if (AIComponent) {
                    AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
                    return;
                }
            }
        }
    }

    PatrolRadiusWander(elapsedTime);
}

void CGhost::OnTraceMove(float elapsedTime)
{
    if (knockback_timer > 0.0f) 
        return;

    auto AIComponent = GetComponent<CAIComponent>();
    auto targetPlayer = target_player.lock();

    if (!targetPlayer || targetPlayer->GetIsPossessed() || targetPlayer->GetReturned()) {
        // 타겟이 사라졌거나 이미 빙의 상태면 IDLE로 복귀
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
    else if (dist <= attack_range && attack_cooldown_timer >= 1.5f) {
        AIComponent->ChangeState(AI_STATE::MONSTER_ATTACK);
        return;
    }
    else if (attack_cooldown_timer < 1.5f) {
        constexpr float retreat_dist = 1.0f;
        if (dist < retreat_dist) {
            XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->position);
            awayDir.y = 0.0f;
            float len = Vector3::Length(awayDir);
            if (len > 0.001f) {
                float yaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
                SetYaw(yaw);
                SetYawPitch(yaw, 0.0f);
                velocity.x = look.x * 1.5f;
                velocity.z = look.z * 1.5f;
                AI_state = AI_STATE::MONSTER_TRACE;
            }
        } else {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            AI_state = AI_STATE::MONSTER_IDLE;
            float yaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
        return;
    }

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

void CGhost::OnAttackMove(float elapsedTime)
{
    if (knockback_timer > 0.0f) 
        return;

    constexpr float DASH_SPEED    = 6.0f;
    constexpr float DASH_DURATION = 0.5f;
    constexpr float STOP_DIST     = 0.7f;

    attack_timer += elapsedTime;
    auto targetPlayer = target_player.lock();

    // 대상이 사라졌거나 빈사/복귀/이미 빙의됨 → 공격 중단하고 IDLE로 복귀
    if (!targetPlayer || targetPlayer->IsIncapacitated() || targetPlayer->GetReturned() || targetPlayer->GetIsPossessed()) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        target_player.reset();
        if (auto ai = GetComponent<CAIComponent>())
            ai->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 돌진: STOP_DIST 이상이면 달리고, 코앞에 닿으면 즉시 멈추고 스턴
    if (attack_timer < DASH_DURATION && targetPlayer) {
        XMFLOAT3 dir = Vector3::Subtract(targetPlayer->position, position);
        dir.y = 0.0f;
        float dist = Vector3::Length(dir);

        if (dist > STOP_DIST) {
            float yaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
            velocity.x = look.x * DASH_SPEED;
            velocity.z = look.z * DASH_SPEED;
        } else {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            if (!stun_applied && targetPlayer->GetIsMyPlayer()) {
                static_cast<CMyPlayer*>(targetPlayer.get())->ApplyStun(1.0f);
                stun_applied = true;
            }
        }
    } else {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
    }

    // 빙의 판정
    if (!hit_damage_dealt && attack_timer >= 1.1f) {
        CSoundManager::GetInstance().Play(SOUND_ID::ghost_attack);
        hit_damage_dealt = true;
        if (targetPlayer && targetPlayer->GetIsMyPlayer() && !targetPlayer->GetIsPossessed()) {
            XMFLOAT3 dir = Vector3::Subtract(targetPlayer->position, position);
            dir.y = 0.0f;
            if (Vector3::Length(dir) <= 0.8f) {
                if (rand() % 100 < 25) {
                    CSoundManager::GetInstance().Play(SOUND_ID::crude_laughter);
                    static_cast<CMyPlayer*>(targetPlayer.get())->ApplyPossession();
                    MarkForDelete();
                }
            }
        }
    }

    if (attack_timer >= 1.27f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
    }
}

void CGhost::OnFleeMove(float elapsedTime)
{
    if (knockback_timer > 0.0f) 
        return;

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

void CGhost::OnIdleEnter()
{
    ResetIdleTimer();
    nav_path.clear();
    path_refresh_timer = 0.0f;
}

void CGhost::OnPatrolEnter()
{
    ResetPatrolTimers();

    stuck_check_timer    = 0.0f;
    last_stuck_pos       = position;
    wander_target        = GetRandomWanderTarget();
    is_waiting           = false;
    wander_wait_timer    = 0.0f;
    wander_wait_duration = 1.0f + (rand() % 11) * 0.1f;
}

void CGhost::OnTraceEnter()
{
    nav_path.clear();
    path_refresh_timer = 0.0f;
    attack_cooldown_timer = 1.2f;
}

void CGhost::OnAttackEnter()
{
    ResetAttackTimer();

    velocity.x       = 0.0f;
    velocity.z       = 0.0f;
    attack_timer          = 0.0f;
    hit_damage_dealt      = false;
    stun_applied          = false;
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

void CGhost::OnFleeEnter()
{
    flee_timer = FLEE_DURATION;
    nav_path.clear();
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

void CGhost::OnFleeExit()
{
    flee_timer = 0.0f;
    velocity.x = 0.0f;
    velocity.z = 0.0f;
}

void CGhost::ApplySprayHit(const XMFLOAT3& fromPos)
{
    CSoundManager::GetInstance().Play(SOUND_ID::devil_scared1);

    target_player = CSceneManager::GetInstance().GetActiveScene()->GetMyPlayer();

    // 넉백
    XMFLOAT3 awayDir = Vector3::Subtract(position, fromPos);
    awayDir.y = 0.0f;
    float len = Vector3::Length(awayDir);
    if (len > 0.001f) {
        knockback_vel   = { awayDir.x / len * KNOCKBACK_FORCE, 0.0f, awayDir.z / len * KNOCKBACK_FORCE };
        knockback_timer = KNOCKBACK_DURATION;
        velocity.x = knockback_vel.x;
        velocity.z = knockback_vel.z;
    }

    spray_hit_count++;

    // 격분: 단계별 속도 증가
    if (spray_hit_count == 1)      
        trace_speed *= 1.3f;
    else if (spray_hit_count == 2) 
        trace_speed *= 1.7f;

    auto* ai = GetComponent<CAIComponent>();
    if (ai) {
        if (spray_hit_count >= MAX_SPRAY_HITS)
            ai->ChangeState(AI_STATE::MONSTER_FLEE);
        else {
            auto cur = ai->GetCurrentState();
            if (!cur || cur->GetType() != AI_STATE::MONSTER_TRACE)
                ai->ChangeState(AI_STATE::MONSTER_TRACE);
        }
    }
}

void CGhost::CheckContactDamage()
{
    if (!g_is_single) 
        return;

    if (AI_state != AI_STATE::MONSTER_ATTACK && AI_state != AI_STATE::MONSTER_FLEE) {
        auto nearPlayer = FindNearestPlayer();
        if (nearPlayer) {
            bool inContact = false;
            auto* ghostCol = GetComponent<CColliderComponent>();
            auto* playerCol = nearPlayer->GetComponent<CColliderComponent>();
            inContact = ghostCol && playerCol && ghostCol->Intersects(playerCol);

            if (inContact) {

                if (contact_damage_timer >= 1.0f) {

                    uint32 hp = nearPlayer->GetHp();
                    nearPlayer->SetHp(hp > 5 ? hp - 5 : 0);
                    contact_damage_timer = 0.0f;

                    if (nearPlayer->GetIsMyPlayer()) {
                        CSoundManager::GetInstance().Play(SOUND_ID::damaged1);
                        XMFLOAT3 knockbackDir = Vector3::Subtract(nearPlayer->position, position);
                        static_cast<CMyPlayer*>(nearPlayer.get())->ApplyKnockback(knockbackDir, 0.6f, 1.0f);
                    }
                }
            }
        }
    }
}
