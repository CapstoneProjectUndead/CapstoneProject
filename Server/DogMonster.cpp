#include "pch.h"
// Server쪽 DogMonster
#include "DogMonster.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "Room.h"
#include "Scene.h"
#include "GameScene.h"
#include "State.h"

CDogMonster::CDogMonster()
	: CMonster(MON_TYPE::ANIMAL_MONSTER)
{
	friction     = 0.0f;
	trace_speed  = 6.0f;
	patrol_speed = 2.0f;
	attack_range = 1.0f;
	SetFOV(180);
	respawn_time = 40.f;
}

CDogMonster::~CDogMonster()
{
}

void CDogMonster::Update(float elapsedTime)
{
	if (give_up_cooldown > 0.0f)
		give_up_cooldown -= elapsedTime;

	CMonster::Update(elapsedTime);

	if (melee_knockback_timer > 0.0f) {
		float ratio = melee_knockback_timer / MELEE_KNOCKBACK_DURATION;
		velocity.x = melee_knockback_vel.x * ratio;
		velocity.z = melee_knockback_vel.z * ratio;
		melee_knockback_timer -= elapsedTime;
		if (melee_knockback_timer < 0.0f) melee_knockback_timer = 0.0f;
	}
}

void CDogMonster::OnIdleMove(float elapsedTime)
{
	if (melee_knockback_timer > 0.0f)
		return;

	// 플레이어 감지 (포기 쿨다운 중에는 스킵)
	auto target = FindNearestPlayer();
	if (target && give_up_cooldown <= 0.0f) {
		XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
		dirVec.y = 0.0f;
		float dist = Vector3::Length(dirVec);

		if (dist <= recog_range && dist > 0.001f) {
			XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };
			float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

			if (dotProduct >= cos_threshold) {
				SetTarget(target);
				auto AIComp = GetComponent<CAIComponent>();
				if (AIComp) {
					AIComp->ChangeState(AI_STATE::MONSTER_TRACE);
					return;
				}
			}
		}
	}

	// origin_position으로 복귀
	XMFLOAT3 dirToOrigin = Vector3::Subtract(origin_position, position);
	dirToOrigin.y = 0.0f;
	float distToOrigin = Vector3::Length(dirToOrigin);

	if (distToOrigin > 0.1f) {
		AI_state = AI_STATE::MONSTER_TRACE;

		const float TILE_SIZE = 2.0f;
		int sx = (int)roundf(position.x / TILE_SIZE);
		int sz = (int)roundf(position.z / TILE_SIZE);
		int ex = (int)roundf(origin_position.x / TILE_SIZE);
		int ez = (int)roundf(origin_position.z / TILE_SIZE);

		path_refresh_timer += elapsedTime;
		if (path_refresh_timer >= 0.3f || nav_path.empty()) {
			path_refresh_timer = 0.0f;
			nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
		}

		XMFLOAT3 moveDir = {};
		bool hasDir = false;

		if (!nav_path.empty()) {
			XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
			XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
			toWp.y = 0.0f;
			if (Vector3::Length(toWp) > 0.1f) {
				moveDir = toWp;
				hasDir  = true;
			}
		}

		// 가까운 거리에서 구조물이 없으면 직접 접근
		if (!hasDir && distToOrigin <= TILE_SIZE * 1.5f) {
			bool blocked = MapGenerator::IsBlockedStructure(ex, ez);
			if (!blocked) {
				int x = sx, z = sz;
				int dx = abs(ex - sx), dz = abs(ez - sz);
				int stepX = (ex > sx) ? 1 : -1;
				int stepZ = (ez > sz) ? 1 : -1;
				int err = dx - dz;
				while (x != ex || z != ez) {
					if (MapGenerator::IsBlockedStructure(x, z)) { blocked = true; break; }
					int e2 = 2 * err;
					if (e2 > -dz) { err -= dz; x += stepX; }
					if (e2 <  dx) { err += dx; z += stepZ; }
				}
			}
			if (!blocked) {
				moveDir = dirToOrigin;
				hasDir  = true;
			}
		}

		if (hasDir) {
			float returnYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
			SetYaw(returnYaw);
			SetYawPitch(returnYaw, 0.0f);
			velocity.x = look.x * PATROL_SPEED;
			velocity.z = look.z * PATROL_SPEED;
			return;
		}

		// 경로 없음 → 현재 위치에서 대기 후 아래 복귀 완료 처리로 낙하
		velocity.x = 0.0f;
		velocity.z = 0.0f;
	}

	// 복귀 완료
	{
		SetYaw(0.0f);
		SetYawPitch(0.0f, 0.0f);
		AI_state = AI_STATE::MONSTER_IDLE;
	}

	velocity.x = 0.0f;
	velocity.z = 0.0f;

	idle_timer += elapsedTime;
	if (idle_timer >= IDLE_DURATION) {
		auto AIComp = GetComponent<CAIComponent>();
		if (AIComp)
			AIComp->ChangeState(AI_STATE::MONSTER_PATROL);
	}
}

void CDogMonster::OnPatrolMove(float elapsedTime)
{
	if (melee_knockback_timer > 0.0f)
		return;

	// 플레이어 감지 (포기 쿨다운 중에는 스킵)
	auto target = FindNearestPlayer();
	if (target && give_up_cooldown <= 0.0f) {
		XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
		dirVec.y = 0.0f;
		float dist = Vector3::Length(dirVec);

		if (dist <= recog_range && dist > 0.001f) {
			XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };
			float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

			if (dotProduct >= cos_threshold) {
				SetTarget(target);
				auto AIComp = GetComponent<CAIComponent>();
				if (AIComp) {
					AIComp->ChangeState(AI_STATE::MONSTER_TRACE);
					return;
				}
			}
		}
	}

	PatrolRadiusWander(elapsedTime);
}

void CDogMonster::OnTraceMove(float elapsedTime)
{
	if (melee_knockback_timer > 0.0f)
		return;

	auto AIComp = GetComponent<CAIComponent>();
	auto targetPlayer = target_player.lock();

	if (!targetPlayer || targetPlayer->GetReturned()) {
		target_player.reset();
		AIComp->ChangeState(AI_STATE::MONSTER_IDLE);
		return;
	}

	dog_bark_time -= elapsedTime;
	if (dog_bark_time <= 0.f && dog_bark) {
		dog_bark = false;
	}

	if (!dog_bark) {
		SendSoundPacket(false, SOUND_ID::dog_bark, GetPosition());
		dog_bark = true;
		dog_bark_time = 1.8f;
	}

	XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->GetPosition(), position);
	dirVec.y = 0.0f;
	float dist = Vector3::Length(dirVec);

	attack_cooldown_timer += elapsedTime;

	if (dist > recog_range) {
		target_player.reset();
		AIComp->ChangeState(AI_STATE::MONSTER_IDLE);
		return;
	}
	else if (dist <= attack_range && attack_cooldown_timer >= 0.4f) {
		AIComp->ChangeState(AI_STATE::MONSTER_ATTACK);
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
	XMFLOAT3 targetPos = targetPlayer->GetPosition();
	int ex = (int)roundf(targetPos.x / TILE_SIZE);
	int ez = (int)roundf(targetPos.z / TILE_SIZE);

	// 항상 BFS로 경로 탐색
	path_refresh_timer += elapsedTime;
	if (path_refresh_timer >= 0.2f || nav_path.empty()) {
		path_refresh_timer = 0.0f;
		nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
	}

	// 유효한 웨이포인트가 있으면 따라간다
	XMFLOAT3 moveDir = {};
	bool hasDir = false;

	if (!nav_path.empty()) {
		XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
		XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
		toWp.y = 0.0f;
		if (Vector3::Length(toWp) > 0.3f) {
			moveDir = toWp;
			hasDir  = true;
		}
	}

	if (!hasDir) {
		// 1.5타일(3.0유닛) 이내이고 직선 경로에 구조물이 없으면 직접 접근
		if (dist <= TILE_SIZE * 1.5f) {
			// 목적지 타일 자체가 구조물이면 즉시 차단 (Bresenham은 목적지를 검사 안 함)
			bool blocked = MapGenerator::IsBlockedStructure(ex, ez);
			if (!blocked) {
				int x = sx, z = sz;
				int dx = abs(ex - sx), dz = abs(ez - sz);
				int stepX = (ex > sx) ? 1 : -1;
				int stepZ = (ez > sz) ? 1 : -1;
				int err = dx - dz;
				while (x != ex || z != ez) {
					if (MapGenerator::IsBlockedStructure(x, z)) {
						blocked = true;
						break;
					}
					int e2 = 2 * err;
					if (e2 > -dz) { err -= dz; x += stepX; }
					if (e2 <  dx) { err += dx; z += stepZ; }
				}
			}
			if (!blocked) {
				moveDir = dirVec;
				hasDir  = true;
			}
		}

		if (!hasDir) {
			// BFS 경로 없음 → 멈추고 포기 타이머 진행
			velocity.x = 0.0f;
			velocity.z = 0.0f;
			path_fail_timer += elapsedTime;
			if (path_fail_timer >= CHASE_GIVE_UP_TIME) {
				path_fail_timer  = 0.0f;
				give_up_cooldown = GIVE_UP_COOLDOWN;
				target_player.reset();
				AIComp->ChangeState(AI_STATE::MONSTER_IDLE);
			}
			return;
		}
	}

	path_fail_timer = 0.0f;

	float targetYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
	SetYaw(targetYaw);
	SetYawPitch(targetYaw, 0.0f);

	velocity.x = look.x * trace_speed;
	velocity.z = look.z * trace_speed;
}

void CDogMonster::OnAttackMove(float elapsedTime)
{
	if (melee_knockback_timer > 0.0f)
		return;

	velocity.x = 0.0f;
	velocity.z = 0.0f;

	// 같은 자리에 박힌 다른 Dog와 분리
	ApplySeparation(0.4f);

	attack_timer += elapsedTime;

	if (!hit_damage_dealt && attack_timer >= ATTACK_HIT_TIME) {
		hit_damage_dealt = true;

		auto targetPlayer = target_player.lock();
		if (targetPlayer) {
			XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->GetPosition(), position);
			dirVec.y = 0.0f;

			XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
			XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

			float forwardDist = Vector3::DotProduct(dirVec, fwd);
			float sideDist    = Vector3::DotProduct(dirVec, right_vec);

			constexpr float depth = 1.2f;
			constexpr float width = 0.7f;

			if (forwardDist >= -0.3f && forwardDist <= depth && fabsf(sideDist) <= width) {
				SendSoundPacket(false, SOUND_ID::damaged1, targetPlayer->GetPosition());

				uint32 hp = targetPlayer->GetHp();
				targetPlayer->SetHp(hp > 8 ? hp - 8 : 0);

				XMFLOAT3 knockbackDir = Vector3::Subtract(targetPlayer->GetPosition(), position);
				targetPlayer->ApplyKnockback(knockbackDir, 0.5f, 0.f);
			}
		}
	}

	if (attack_timer >= ATTACK_DURATION) {
		auto AIComp = GetComponent<CAIComponent>();
		if (AIComp)
			AIComp->ChangeState(AI_STATE::MONSTER_TRACE);
	}
}

void CDogMonster::OnIdleEnter()
{
	ResetIdleTimer();
	nav_path.clear();
	path_refresh_timer = 0.0f;
}

void CDogMonster::OnPatrolEnter()
{
	ResetPatrolTimers();

	stuck_check_timer    = 0.0f;
	last_stuck_pos       = position;
	wander_target        = GetRandomWanderTarget();
	is_waiting           = false;
	wander_wait_timer    = 0.0f;
	wander_wait_duration = 1.0f + (rand() % 11) * 0.1f;
}

void CDogMonster::OnTraceEnter()
{
	nav_path.clear();
	path_refresh_timer = 0.0f;
	path_fail_timer    = 0.0f;
}

void CDogMonster::OnAttackEnter()
{
	ResetAttackTimer();

	velocity.x            = 0.0f;
	velocity.z            = 0.0f;
	attack_timer          = 0.0f;
	hit_damage_dealt      = false;
	attack_cooldown_timer = 0.0f;

	auto targetPlayer = target_player.lock();
	if (targetPlayer) {
		XMFLOAT3 dir = Vector3::Subtract(targetPlayer->GetPosition(), position);
		dir.y = 0.0f;
		float len = Vector3::Length(dir);
		if (len > 0.001f) {
			float fYaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
			SetYaw(fYaw);
			SetYawPitch(fYaw, 0.0f);
		}
	}
}

void CDogMonster::OnFleeEnter()
{
	SendSoundPacket(false, SOUND_ID::dog_howling, GetPosition());

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
		XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->GetPosition());
		awayDir.y = 0.0f;
		float len = Vector3::Length(awayDir);
		if (len > 0.001f) {
			float fYaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
			SetYaw(fYaw);
			SetYawPitch(fYaw, 0.0f);
		}
	}

	DropItem();
}

void CDogMonster::OnFleeMove(float elapsedTime)
{
	if (melee_knockback_timer > 0.0f) return;

	flee_timer -= elapsedTime;
	if (flee_timer <= 0.0f) {
		MarkForDelete();
		return;
	}

	auto targetPlayer = target_player.lock();
	if (targetPlayer) {
		XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->GetPosition());
		awayDir.y = 0.0f;
		float len = Vector3::Length(awayDir);
		if (len > 0.001f) {
			float fYaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
			SetYaw(fYaw);
			SetYawPitch(fYaw, 0.0f);
		}
	}

	velocity.x = look.x * FLEE_SPEED;
	velocity.z = look.z * FLEE_SPEED;
}

void CDogMonster::OnFleeExit()
{
	flee_timer = 0.0f;
	velocity.x = 0.0f;
	velocity.z = 0.0f;
}
