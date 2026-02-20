#include "stdafx.h"
#include "Animator.h"
#include "GeometryLoader.h"
#include "Object.h"
#include "Movement.h"
#include "Player.h"

// animator
void CAnimatorComponent::Initialize(const std::string& charName, const std::string& AniName)
{
	SkeletonData skeleton = CGeometryLoader::LoadSkeleton(charName);
	auto animData = CGeometryLoader::LoadAnimations(AniName, skeleton.bone_names.size());
	skinned.Set(skeleton.parent_index, skeleton.inverse_bind_pose, animData);
}

void CAnimatorComponent::Play(const std::string& name)
{
	if (current_animation != name) {
		current_animation = name;
		current_time = 0.0f;
	}
}

void CAnimatorComponent::Update(float deltaTime)
{
	if (current_animation.empty())
		return;

	if (owner == nullptr)
		return;

	auto move = owner->GetComponent<CMovementComponent>();
	float speed = 0.0f;

	if (move)
		speed = Vector3::Length(owner->velocity);

	auto p = dynamic_cast<CPlayer*>(owner);
	if (p == nullptr)
		return;

	// 내 플레이어
	if (p->GetIsMyPlayer()) {

		if (speed < 0.01f)
			Play("Ganga_idle");
		else
			Play("Ganga_walk");

	}
	// 상대 플레이어
	// 상대 플레이어는 속도가 아니라 서버가 알려준 state 상태로 판단하다.
	else {
		if (p->GetState() == PLAYER_STATE::IDLE)
			Play("Ganga_idle");
		else if (p->GetState() == PLAYER_STATE::WALK)
			Play("Ganga_walk");
	}


	current_time += deltaTime;

	float start = skinned.GetClipStartTime(current_animation);
	float end = skinned.GetClipEndTime(current_animation);

	// 루프 처리
	if (current_time > end)
		current_time = start;

	// 본 행렬 계산
	final_transforms.resize(skinned.BoneCount());
	skinned.GetFinalTransforms(current_animation, current_time, final_transforms, owner->pitch);
}

void CAnimatorComponent::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!final_transforms.empty()) {
		memcpy(mapped, final_transforms.data(), sizeof(XMFLOAT4X4) * final_transforms.size());
	}

	commandList->SetGraphicsRootConstantBufferView(4, skinned_cb->GetGPUVirtualAddress());
}

void CAnimatorComponent::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	skinned_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<SkinnedDataCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	skinned_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}