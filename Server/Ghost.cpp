#include "pch.h"
// Server쪽 Ghost
#include "Ghost.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "SceneManager.h"
#include "Collider.h"

CGhost::CGhost()
	: CMonster(MON_TYPE::GHOST)
	, stuck_check_timer(0.0f)
	, last_stuck_pos{}
	, wander_target{}
	, wander_wait_timer(0.0f)
	, wander_wait_duration(1.5f)
	, is_waiting(false)
{
	friction = 0.0f;
	trace_speed = 2.0f;
	attack_range = 0.4f;
	SetFOV(120);
}

CGhost::~CGhost()
{
}

void CGhost::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);

	contact_damage_timer += elapsedTime;

	auto nearPlayer = FindNearestPlayer();
	if (nearPlayer) {

		auto* ghostCol  = GetComponent<CColliderComponent>();
		auto* playerCol = nearPlayer->GetComponent<CColliderComponent>();

		if (ghostCol && playerCol && ghostCol->Intersects(playerCol)) {

			if (contact_damage_timer >= 1.0f) {
				uint32 hp = nearPlayer->GetHp();
				nearPlayer->SetHp(hp > 10 ? hp - 10 : 0);
				contact_damage_timer = 0.0f;

				XMFLOAT3 knockbackDir = Vector3::Subtract(nearPlayer->GetPosition(), position);
				nearPlayer->ApplyKnockback(knockbackDir, 0.6f);
			}
		}
	}
}

void CGhost::OnIdleMove(float elapsedTime)
{
	auto target = FindNearestPlayer();

	if (target) {
		XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
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

		float walk_speed = 0.5f;
		velocity.x = look.x * walk_speed;
		velocity.z = look.z * walk_speed;
		return;
	}

	{
		SetYaw(0.0f);
		SetYawPitch(0.0f, 0.0f);
		AI_state = AI_STATE::MONSTER_IDLE;
	}

	velocity.x = 0.0f;
	velocity.z = 0.0f;

	idle_timer += elapsedTime;
	if (idle_timer >= 5.0f) {
		auto AIComponent = GetComponent<CAIComponent>();
		if (AIComponent)
			AIComponent->ChangeState(AI_STATE::MONSTER_PATROL);
	}
}

void CGhost::OnPatrolMove(float elapsedTime)
{
	auto target = FindNearestPlayer();
	if (target) {
		XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
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
	auto AIComponent = GetComponent<CAIComponent>();

	if (!target_player) {
		AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
		return;
	}

	XMFLOAT3 dirVec = Vector3::Subtract(target_player->GetPosition(), position);
	dirVec.y = 0.0f;
	float dist = Vector3::Length(dirVec);

	if (dist > recog_range) {
		target_player = nullptr;
		AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
		return;
	}
	else if (dist <= attack_range) {
		AIComponent->ChangeState(AI_STATE::MONSTER_ATTACK);
		return;
	}

	const float TILE_SIZE = 2.0f;
	int sx = (int)roundf(position.x / TILE_SIZE);
	int sz = (int)roundf(position.z / TILE_SIZE);
	int ex = (int)roundf(target_player->GetPosition().x / TILE_SIZE);
	int ez = (int)roundf(target_player->GetPosition().z / TILE_SIZE);

	// Bresenham 직선으로 벽 여부 확인 — 막힘 없으면 직진, 막히면 BFS
	bool has_wall = false;
	{
		int x = sx, z = sz;
		int dx = abs(ex - sx), dz = abs(ez - sz);
		int stepX = (ex > sx) ? 1 : -1;
		int stepZ = (ez > sz) ? 1 : -1;
		int err = dx - dz;

		while (x != ex || z != ez) {
			if (MapGenerator::IsBlockedStructure(x, z)) { has_wall = true; break; }
			int e2 = 2 * err;
			if (e2 > -dz) { err -= dz; x += stepX; }
			if (e2 <  dx) { err += dx; z += stepZ; }
		}
	}

	XMFLOAT3 moveDir = dirVec;
	if (has_wall) {
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
	velocity.x = 0.0f;
	velocity.z = 0.0f;

	attack_timer += elapsedTime;

	if (!hit_damage_dealt && attack_timer >= 0.6f) {
		hit_damage_dealt = true;
		if (target_player) {
			XMFLOAT3 dirVec = Vector3::Subtract(target_player->GetPosition(), position);
			dirVec.y = 0.0f;

			XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
			XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

			float forwardDist = Vector3::DotProduct(dirVec, fwd);
			float sideDist    = Vector3::DotProduct(dirVec, right_vec);

			constexpr float depth = 2.0f;
			constexpr float width = 1.5f;

			if (forwardDist >= 0.0f && forwardDist <= depth && fabsf(sideDist) <= width) {
				if (rand() % 100 < 30) {
					target_player->ApplyPossession();
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
}

void CGhost::OnAttackEnter()
{
	ResetAttackTimer();

	velocity.x       = 0.0f;
	velocity.z       = 0.0f;
	attack_timer     = 0.0f;
	hit_damage_dealt = false;
}

void CGhost::PatrolRadiusWander(float elapsedTime)
{
	const float arrive_threshold     = 0.3f;
	const float stuck_check_interval = 0.1f;
	const float stuck_threshold      = 0.03f;

	if (is_waiting) {

		velocity.x = 0.0f;
		velocity.z = 0.0f;

		wander_wait_timer += elapsedTime;

		if (wander_wait_timer >= wander_wait_duration) {
			wander_target        = GetRandomWanderTarget();
			wander_wait_duration = 1.0f + (rand() % 11) * 0.1f;
			is_waiting           = false;
			wander_wait_timer    = 0.0f;
			stuck_check_timer    = 0.0f;
			last_stuck_pos       = position;
		}
		return;
	}

	stuck_check_timer += elapsedTime;
	if (stuck_check_timer >= stuck_check_interval) {
		XMFLOAT3 moved = Vector3::Subtract(position, last_stuck_pos);
		moved.y = 0.0f;
		if (Vector3::Length(moved) < stuck_threshold) {
			wander_target = GetRandomWanderTarget();
		}
		stuck_check_timer = 0.0f;
		last_stuck_pos    = position;
	}

	XMFLOAT3 dirVec = Vector3::Subtract(wander_target, position);
	dirVec.y = 0.0f;
	float dist = Vector3::Length(dirVec);

	if (dist < arrive_threshold) {
		velocity.x        = 0.0f;
		velocity.z        = 0.0f;
		is_waiting        = true;
		wander_wait_timer = 0.0f;
		return;
	}

	float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
	SetYaw(targetYaw);
	SetYawPitch(targetYaw, 0.0f);

	velocity.x = look.x * patrol_speed;
	velocity.z = look.z * patrol_speed;
}

XMFLOAT3 CGhost::GetRandomWanderTarget()
{
	const float TILE_SIZE = 2.0f;

	int cx = (int)roundf(position.x / TILE_SIZE);
	int cz = (int)roundf(position.z / TILE_SIZE);

	const int dx[] = { 0, 0, -1, 1 };
	const int dz[] = { -1, 1,  0, 0 };

	std::vector<MapGenerator::Cell> candidates;
	for (int i = 0; i < 4; i++) {

		int nx = cx + dx[i];
		int nz = cz + dz[i];

		if (MapGenerator::IsWalkableFloor(nx, nz) && !MapGenerator::IsBlockedStructure(nx, nz))
			candidates.push_back({ nx, nz });
	}

	if (candidates.empty())
		return position;

	int idx = rand() % (int)candidates.size();

	return { candidates[idx].x * TILE_SIZE, position.y, candidates[idx].y * TILE_SIZE };
}
