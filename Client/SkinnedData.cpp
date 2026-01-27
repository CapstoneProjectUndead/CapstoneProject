#include "stdafx.h"
#include "SkinnedData.h"
#include "GeometryLoader.h"

Keyframe::Keyframe()
	: time_pos{ 0.0f },
	translation{ 0.0f, 0.0f, 0.0f },
	scale{ 1.0f, 1.0f, 1.0f },
	rotation{ 0.0f, 0.0f, 0.0f, 1.0f }
{
}

float BoneAnimation::GetStartTime()const
{
	return key_frames.front().time_pos;
}

float BoneAnimation::GetEndTime()const
{
	float f = key_frames.back().time_pos;

	return f;
}

void BoneAnimation::Interpolate(float t, XMFLOAT4X4& M)const
{
	if (t <= key_frames.front().time_pos)
	{
		XMVECTOR S = XMLoadFloat3(&key_frames.front().scale);
		XMVECTOR P = XMLoadFloat3(&key_frames.front().translation);
		XMVECTOR Q = XMLoadFloat4(&key_frames.front().rotation);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (t >= key_frames.back().time_pos)
	{
		XMVECTOR S = XMLoadFloat3(&key_frames.back().scale);
		XMVECTOR P = XMLoadFloat3(&key_frames.back().translation);
		XMVECTOR Q = XMLoadFloat4(&key_frames.back().rotation);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		for (UINT i = 0; i < key_frames.size() - 1; ++i)
		{
			if (t >= key_frames[i].time_pos && t <= key_frames[i + 1].time_pos)
			{
				float lerpPercent = (t - key_frames[i].time_pos) / (key_frames[i + 1].time_pos - key_frames[i].time_pos);

				XMVECTOR s0 = XMLoadFloat3(&key_frames[i].scale);
				XMVECTOR s1 = XMLoadFloat3(&key_frames[i + 1].scale);

				XMVECTOR p0 = XMLoadFloat3(&key_frames[i].translation);
				XMVECTOR p1 = XMLoadFloat3(&key_frames[i + 1].translation);

				XMVECTOR q0 = XMLoadFloat4(&key_frames[i].rotation);
				XMVECTOR q1 = XMLoadFloat4(&key_frames[i + 1].rotation);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}

float AnimationClip::GetClipStartTime()const
{
	// Find smallest start time over all bones in this clip.
	float t = FLT_MAX;
	for (UINT i = 0; i < bone_animations.size(); ++i)
	{
		t = Math::Min(t, bone_animations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime()const
{
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	for (UINT i = 0; i < bone_animations.size(); ++i)
	{
		t = Math::Max(t, bone_animations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms)const
{
	for (UINT i = 0; i < bone_animations.size(); ++i)
	{
		bone_animations[i].Interpolate(t, boneTransforms[i]);
	}
}

float CSkinnedData::GetClipStartTime(const std::string& clipName)const
{
	auto clip = animations.find(clipName);
	return clip->second.GetClipStartTime();
}

float CSkinnedData::GetClipEndTime(const std::string& clipName)const
{
	auto clip = animations.find(clipName);
	return clip->second.GetClipEndTime();
}

UINT CSkinnedData::BoneCount()const
{
	return bone_hierarchy.size();
}

void CSkinnedData::Set(std::vector<int>& boneHierarchy,	std::vector<XMFLOAT4X4>& boneOffsets, std::unordered_map<std::string, AnimationClip>& otherAnimations)
{
	bone_hierarchy = boneHierarchy;
	bone_offsets = boneOffsets;
	animations = otherAnimations;
}

void CSkinnedData::GetFinalTransforms(const std::string& clipName, float timePos, std::vector<XMFLOAT4X4>& finalTransforms)const
{
	UINT numBones = bone_offsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);

	// Interpolate all the bones of this clip at the given time instance.
	auto clip = animations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	//
	// Traverse the hierarchy and transform all the bones to the root space.
	//

	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// The root bone has index 0.  The root bone has no parent, so its toRootTransform
	// is just its local bone transform.
	toRootTransforms[0] = toParentTransforms[0];

	// Now find the toRootTransform of the children.
	for (UINT i = 1; i < numBones; ++i)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

		int parentIndex = bone_hierarchy[i];
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// Premultiply by the bone offset transform to get the final transform.
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&bone_offsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
}

// animator
void CAnimator::Initialize(const std::string& charName, const std::string& AniName)
{
	auto skeleton = CGeometryLoader::LoadSkeleton(charName);

	auto animData = CGeometryLoader::LoadAnimations(AniName, skeleton.bone_names.size());
	skinned.Set(skeleton.parent_index, skeleton.inverse_bind_pose, animData);
}

void CAnimator::Play(const std::string& name)
{
	if (current_animation != name) {
		current_animation = name;
		current_time = 0.0f;
	}
}

void CAnimator::Update(float deltaTime)
{
	if (current_animation.empty())
		return;

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

void CAnimator::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	SkinnedDataCB cb{};
	cb.skinned = true;
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

void CAnimator::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SkinnedDataCB cb{};
	cb.skinned = true;
	skinned_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<SkinnedDataCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
}