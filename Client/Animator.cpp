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
}

void CAnimatorComponent::Play(const std::string& name)
{
	if (current_animation != name) {
		current_animation = name;
		current_time = 0.0f;
	}
}

XMVECTOR CAnimatorComponent::GetHeadPosition()
{
	auto& clip = CAnimationManager::GetInstance().GetClip(current_animation);

	if (clip.head_bone_idx == -1) return XMVECTOR{};

	XMVECTOR headPos = CAnimationManager::GetInstance().GetBoneWorldPos(
		current_animation,
		current_time,
		clip.head_bone_idx
	);

	return headPos;
}

AnimationData CAnimatorComponent::GetAnimationData()
{
	AnimationData data{};

	// AnimationManager를 통해 현재 재생 중인 클립의 정보를 가져옴
	auto& clip = CAnimationManager::GetInstance().GetClip(current_animation);

	data.start_offset = clip.start_matrix_offset;
	data.bone_count = clip.bone_count; // 혹은 미리 저장된 본 개수

	// 60FPS 고정 샘플링이므로 현재 시간 기반 프레임 계산
	uint32_t totalFrames = clip.total_frames;
	data.cur_frame = (uint32_t)(current_time * 60.0f) % totalFrames;

	return data;
}

void CAnimatorComponent::Update(float deltaTime)
{
	if (current_animation.empty())
		return;

	if (owner == nullptr)
		return;

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

	current_time += deltaTime;

	// 루프 처리
	/*if (current_time > end)
		current_time = start;*/
}

void CAnimatorComponent::UpdatePlayerAnimation()
{
	auto player = static_cast<CPlayer*>(owner);
	if (player == nullptr)
		return;

	// 내 플레이어
	if (player->GetIsMyPlayer()) {

		auto move = owner->GetComponent<CMovementComponent>();
		float speed = 0.0f;

		if (move)
			speed = Vector3::Length(owner->velocity);

		if (speed < 0.3f)
			Play("Ganga_idle");
		else
			Play("Ganga_walk");
	}
	// 상대 플레이어
	// 상대 플레이어는 속도가 아니라 서버가 알려준 state 상태로 판단하다.
	else {
		if (player->GetState() == PLAYER_STATE::IDLE)
			Play("Ganga_idle");
		else if (player->GetState() == PLAYER_STATE::WALK)
			Play("Ganga_walk");
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
		if (state == AI_STATE::MONSTER_IDLE) {
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
		}
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