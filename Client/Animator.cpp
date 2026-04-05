#include "stdafx.h"
#include "Animator.h"
#include "Object.h"
#include "Player.h"
#include "Monster.h"
#include "Movement.h"
#include "AnimationManager.h"
#include "GPUBufferStruct.h"

CAnimatorComponent::CAnimatorComponent()
{
	layers.resize(3);
	layers[0].mask_id = -1; // 베이스는 전체
	layers[1].mask_id = 0;
	layers[2].mask_id = 1;

	//PlayAction("Ganga_expect");
	CharacterAnimSet playerSet = { "Ganga_idle", "Ganga_walk", "Ganga_run", "Ganga_expect" };
	Init(playerSet);
}

void CAnimatorComponent::Init(const CharacterAnimSet& animSet)
{
	anim_set = animSet;

	// 상태 등록
	controller.AddState({ "IdleState", animSet.idle });
	controller.AddState({ "MoveState", animSet.walk });
	controller.AddState({ "RunState", animSet.run });

	// 전이 규칙 (중복 코드를 줄이기 위해 내부 함수 활용 가능)
	AddLocomotionTransitions("IdleState", "MoveState", "RunState");
}

void CAnimatorComponent::AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run)
{
	// 히스테리시스 적용
	const float idleToWalkThres = 0.35f; // 정지에서 걷기 시작 (높게)
	const float walkToIdleThres = 0.20f; // 걷다가 정지 (낮게)

	const float walkToRunThres = 0.80f;
	const float runToWalkThres = 0.65f;

	// Idle -> Walk
	Transition i2w;
	i2w.to_state = walk;
	i2w.duration = 0.2f;
	i2w.condition = [this, idleToWalkThres]() {
		return Vector3::Length(owner->velocity) > idleToWalkThres;
		};
	controller.AddTransition(idle, i2w);

	// Walk -> Idle
	Transition w2i;
	w2i.to_state = idle;
	w2i.duration = 0.2f;
	w2i.condition = [this, walkToIdleThres]() {
		return Vector3::Length(owner->velocity) < walkToIdleThres;
		};
	controller.AddTransition(walk, w2i);

	// Walk -> Run
	Transition w2r;
	w2r.to_state = run;
	w2r.duration = 0.2f;
	w2r.condition = [this, walkToRunThres]() { return Vector3::Length(owner->velocity) > walkToRunThres; };
	controller.AddTransition(walk, w2r);

	// Run -> Walk
	Transition r2w;
	r2w.to_state = walk;
	r2w.duration = 0.2f;
	r2w.condition = [this, runToWalkThres]() { return Vector3::Length(owner->velocity) < runToWalkThres; };
	controller.AddTransition(run, r2w);
}

void CAnimatorComponent::PlayAction(const std::string& clipName)
{
	if (layers[1].current_clip == clipName) return;

	layers[1].current_clip = clipName;
	// 현재 흐르고 있는 전체 시간을 '시작 시간'으로 박제!
	layers[1].start_time = current_time;
	layers[1].weight = 0.0f;
}

XMVECTOR CAnimatorComponent::GetHeadPosition()
{
	// 현재 실제로 베이스 레이어에서 재생 중인 클립을 가져옴
	std::string activeClip = layers[0].current_clip;
	if (activeClip.empty()) return XMVECTOR{};

	auto& clip = CAnimationManager::GetInstance().GetClip(activeClip);
	if (clip.head_bone_idx == -1) return XMVECTOR{};

	float relativeTime = current_time - layers[0].start_time;
	return CAnimationManager::GetInstance().GetBoneWorldPos(activeClip, relativeTime, clip.head_bone_idx);
}

void CAnimatorComponent::UpdateLayerWeights(float deltaTime)
{
	// Layer 0 (Base)는 항상 Weight 1.0이므로 관리 생략

	// Layer 1 (Action/UpperBody) 페이드 인/아웃
	if (!layers[1].current_clip.empty()) {
		layers[1].weight += deltaTime * 5.0f; // 0.2초 Fade-in
		if (layers[1].weight > 1.0f) layers[1].weight = 1.0f;
	}
	else {
		layers[1].weight -= deltaTime * 5.0f; // Fade-out
		if (layers[1].weight < 0.0f) layers[1].weight = 0.0f;
	}
}

AnimationData CAnimatorComponent::GetAnimationData()
{
	AnimationData data{};
	auto& manager = CAnimationManager::GetInstance();

	// 베이스 애니메이션 (Layer 0)
	std::string baseClip = layers[0].current_clip;
	if (baseClip.empty()) return data;

	auto& animA = manager.GetClip(baseClip);
	data.start_offset_A = animA.start_matrix_offset;

	float relativeTimeA = current_time - layers[0].start_time;
	data.cur_frame_A = (uint32_t)(relativeTimeA * 60.0f) % animA.total_frames;
	data.bone_count = animA.bone_count;

	// 블렌딩 대상 결정
	// 우선순위 1: 상반신 액션(감정 표현 모션 등)
	if (!layers[1].current_clip.empty() && layers[1].weight > 0.0f) {
		// 상반신 액션 모드
		auto& animB = manager.GetClip(layers[1].current_clip);
		data.start_offset_B = animB.start_matrix_offset;
		float relativeTimeB = current_time - layers[1].start_time;
		data.cur_frame_B = (uint32_t)(relativeTimeB * 60.0f) % animB.total_frames;

		data.blend_weight = layers[1].weight;
		data.mask_id = layers[1].mask_id;
	}
	// 우선순위 2: 상태 전이 중
	else if (controller.IsBlending()) {
		std::string clipB = controller.GetNextClip();
		if (!clipB.empty()) {
			auto& animB = manager.GetClip(clipB);
			data.start_offset_B = animB.start_matrix_offset;
			data.cur_frame_B = (uint32_t)(current_time * 60.0f) % animB.total_frames;
			data.blend_weight = controller.GetWeight();
			data.mask_id = -1;
		}
	}
	else {
		data.blend_weight = 0.0f;
		data.mask_id = -1;
	}

	return data;
}

void CAnimatorComponent::Update(float deltaTime)
{
	if (current_animation.empty())
		return;

	if (owner == nullptr)
		return;

	current_time += deltaTime;
	
	// 애니메이션 상태 머신 update
	controller.Update(deltaTime);
	UpdateLayerWeights(deltaTime);

	// base 동기화 및 업데이트
	std::string newBase = controller.GetCurrentClip();
	if (layers[0].current_clip != newBase) {
		layers[0].current_clip = newBase;
		layers[0].start_time = current_time; // 베이스도 바뀔 때마다 0초부터 재생되게 함
	}

	// 타입이 플레이어? or 몬스터?
	OBJECT_TYPE type = owner->GetObjectType();

	switch (type)
	{
	case OBJECT_TYPE::PLAYER:
		UpdatePlayerAnimation();
		break;
	case OBJECT_TYPE::MONSTER:
		UpdateMonsterAnimation();
		break;
	default:
		break;
	}
}

void CAnimatorComponent::UpdatePlayerAnimation()
{
	auto player = static_cast<CPlayer*>(owner);
	if (player == nullptr)
		return;

	// 내 플레이어
	if (player->GetIsMyPlayer()) {

		/*auto move = owner->GetComponent<CMovementComponent>();
		float speed = 0.0f;

		if (move)
			speed = Vector3::Length(owner->velocity);

		if (speed < 0.3f)
			Play("Ganga_idle");
		else
			Play("Ganga_walk");*/
	}
	// 상대 플레이어
	// 상대 플레이어는 속도가 아니라 서버가 알려준 state 상태로 판단하다.
	else {
		/*if (player->GetState() == PLAYER_STATE::IDLE)
			Play("Ganga_idle");
		else if (player->GetState() == PLAYER_STATE::WALK)
			Play("Ganga_walk");*/
	}
}

void CAnimatorComponent::UpdateMonsterAnimation()
{
	auto monster = static_cast<CMonster*>(owner);

	MON_TYPE type = monster->GetMonsterType();
	AI_STATE state = monster->GetAIState();

	switch (type)
	{
	case MON_TYPE::HUMAN_MONSTER:
	{
		/*if (state == AI_STATE::MONSTER_IDLE) {
			Play("Ganga_idle");
		}
		else if (state == AI_STATE::MONSTER_PATROL) {
			Play("Ganga_walk");
		}
		else if (state == AI_STATE::MONSTER_TRACE) {
			Play("Ganga_walk");
		}
		else if (state == AI_STATE::MONSTER_ATTACK) {
			Play("Ganga_walk");
		}*/
	}
	break;
	case MON_TYPE::ANIMAL_MONSTER:
		break;
	case MON_TYPE::GHOST:
		break;
	default:
		break;
	}
}