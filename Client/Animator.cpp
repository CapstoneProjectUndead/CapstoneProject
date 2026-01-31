#include "stdafx.h"
#include "Animator.h"
#include "GeometryLoader.h"
#include "Object.h"
#include "Movement.h"

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

	auto move = owner->GetComponent<CMovementComponent>();
	float speed = 0.0f;

	if (move)
		speed = Vector3::Length(owner->velocity);

	if (speed < 0.01f)
		Play("Ganga_idle");
	else
		Play("Ganga_walk");


	current_time += deltaTime;

	float start = skinned.GetClipStartTime(current_animation);
	float end = skinned.GetClipEndTime(current_animation);

	// 루프 처리
	if (current_time > end)
		current_time = start;

	// 본 행렬 계산
	final_transforms.resize(skinned.BoneCount());
	skinned.GetFinalTransforms(current_animation, current_time, final_transforms);
}

void CAnimatorComponent::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	SkinnedDataCB cb{};
	if (!final_transforms.empty()) {
		UINT boneSize = skinned.BoneCount();
		for (UINT i = 0; i < boneSize; ++i)
			cb.bone_transforms[i] = final_transforms[i];
	}

	UINT8* mapped = nullptr;
	skinned_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &cb, sizeof(cb));
	skinned_cb->Unmap(0, nullptr);

	commandList->SetGraphicsRootConstantBufferView(4, skinned_cb->GetGPUVirtualAddress());
}

void CAnimatorComponent::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SkinnedDataCB cb{};
	skinned_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<SkinnedDataCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
}