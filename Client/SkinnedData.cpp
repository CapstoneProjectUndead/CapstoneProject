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
		t = min(t, bone_animations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime()const
{
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	for (UINT i = 0; i < bone_animations.size(); ++i)
	{
		t = max(t, bone_animations[i].GetEndTime());
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

void CSkinnedData::Set(const std::vector<int>& boneHierarchy, const std::vector<XMFLOAT4X4>& boneOffsets, const std::unordered_map<std::string, AnimationClip>& otherAnimations)
{
	bone_hierarchy = boneHierarchy;
	bone_offsets = boneOffsets;
	animations = otherAnimations;
}

void CSkinnedData::GetFinalTransforms(const std::string& clipName, float timePos, std::vector<XMFLOAT4X4>& finalTransforms, const float pitch,
	float elapsedTime, DynamicBoneChain* leftEar, DynamicBoneChain* rightEar, DynamicBoneChain* tail)
{
	UINT numBones = bone_offsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);

	// Interpolate all the bones of this clip at the given time instance.
	auto clip = animations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	// 고개 움직임 적용
	int spineIdx = 2;  // 허리
	int chestIdx = 3;  // 가슴
	int neckIdx = 4;  // 목

	float pitchRad = XMConvertToRadians(std::clamp(pitch, -30.0f, 70.0f));
	float weights[] = { 0.2f, 0.3f, 0.5f };	// 합 1.0
	int targetIndices[] = { spineIdx, chestIdx, neckIdx };

	for (int i = 0; i < 3; ++i) {
		int idx = targetIndices[i];

		XMMATRIX localM = XMLoadFloat4x4(&toParentTransforms[idx]);

		// 가중치 적용
		XMMATRIX rotation = XMMatrixRotationZ(pitchRad * weights[i]);

		// 기존 애니메이션 행렬에 마우스 회전 결합
		XMStoreFloat4x4(&toParentTransforms[idx], XMMatrixMultiply(rotation, localM));
	}

	//
	// Traverse the hierarchy and transform all the bones to the root space.
	//

	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// The root bone has index 0.  The root bone has no parent, so its toRootTransform
	// is just its local bone transform.
	toRootTransforms[0] = toParentTransforms[0];



	// Now find the toRootTransform of the children.
	for (UINT i = 1; i < numBones; ++i) {
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

		int parentIndex = bone_hierarchy[i];
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// =======================================================
		// 🌟 [수정됨] toParentTransforms 변수도 같이 넘겨줍니다!
	if (leftEar) SimulateChain(*leftEar, toRootTransforms, toParentTransforms, elapsedTime);
	if (rightEar) SimulateChain(*rightEar, toRootTransforms, toParentTransforms, elapsedTime);
	if (tail) SimulateChain(*tail, toRootTransforms, toParentTransforms, elapsedTime);
	// =======================================================

	// Premultiply by the bone offset transform to get the final transform.
	XMMATRIX rotate = XMMatrixRotationY(XM_PI);	// 180도 회전
	for (UINT i = 0; i < numBones; ++i) {
		XMMATRIX offset = XMLoadFloat4x4(&bone_offsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);

		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMMATRIX inversefinalTransform = XMMatrixMultiply(finalTransform, rotate);

		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(inversefinalTransform));
	}

	int headIdx = 5;
	XMMATRIX headToRoot = XMLoadFloat4x4(&toRootTransforms[headIdx]); 
	XMMATRIX localEyeMatrix = headToRoot * rotate;
	XMStoreFloat3(&head_position, localEyeMatrix.r[3]);
}

void CSkinnedData::SimulateChain(DynamicBoneChain& chain, std::vector<XMFLOAT4X4>& toRootTransforms, const std::vector<XMFLOAT4X4>& toParentTransforms, float elapsedTime)
{
	if (chain.bone_indices.empty()) return;

	XMVECTOR gravity = XMVectorSet(0.0f, -1.2f, 0.0f, 0.0f);
	float stiffness = 70.0f; // 빳빳한 정도
	float damping = 0.85f;    // 마찰력

	// 0번 구슬(뿌리)은 원래 위치에 꽉 고정!
	int rootIdx = chain.bone_indices[0];
	XMMATRIX rootMatrix = XMLoadFloat4x4(&toRootTransforms[rootIdx]);
	XMStoreFloat3(&chain.nodes[0].current_position, rootMatrix.r[3]);

	for (size_t i = 1; i < chain.bone_indices.size(); ++i)
	{
		int pIdx = chain.bone_indices[i - 1]; // 부모 뼈 번호
		int cIdx = chain.bone_indices[i];     // 내(자식) 뼈 번호

		// 1. 방금 전(i-1)에 업데이트 된 '최신 부모 행렬' 가져오기
		XMMATRIX parentMatrix = XMLoadFloat4x4(&toRootTransforms[pIdx]);
		XMVECTOR parentPos = parentMatrix.r[3];

		// 2. 내 원래 목표 위치 구하기 (중요: 탈골 방지!)
		// 내 원래 지역 행렬을 '최신 부모'에 곱해서, 부모가 꺾인 만큼 나도 따라간 위치를 목표로 삼음!
		XMMATRIX childLocal = XMLoadFloat4x4(&toParentTransforms[cIdx]);
		XMMATRIX childTargetMatrix = XMMatrixMultiply(childLocal, parentMatrix);
		XMVECTOR targetPos = childTargetMatrix.r[3];

		// 3. 물리 연산 (통통 튀기기)
		XMVECTOR currentPos = XMLoadFloat3(&chain.nodes[i].current_position);
		XMVECTOR velocity = XMLoadFloat3(&chain.nodes[i].velocity);

		XMVECTOR force = (targetPos - currentPos) * stiffness;
		force += gravity;

		velocity += force * elapsedTime;
		velocity *= damping;
		currentPos += velocity * elapsedTime;

		// 고무줄 방지
		XMVECTOR direction = XMVector3Normalize(currentPos - parentPos);
		currentPos = parentPos + (direction * chain.bone_length);

		XMStoreFloat3(&chain.nodes[i].current_position, currentPos);
		XMStoreFloat3(&chain.nodes[i].velocity, velocity);

		// 4. 부모 뼈를 내 쪽으로 살짝 비틀기 (꽈배기 방지 쿼터니언 마법!)
		XMVECTOR origDir = XMVector3Normalize(targetPos - parentPos);
		XMVECTOR simDir = direction;

		XMVECTOR cross = XMVector3Cross(origDir, simDir);
		float d = XMVectorGetX(XMVector3Dot(origDir, simDir));

		XMMATRIX rotMatrix;
		if (d > 0.9999f) {
			rotMatrix = XMMatrixIdentity();
		}
		else if (d < -0.999f) {
			rotMatrix = XMMatrixRotationX(XM_PI);
		}
		else {
			float s = sqrtf((1.0f + d) * 2.0f);
			float invs = 1.0f / s;
			XMVECTOR q = XMVectorSet(XMVectorGetX(cross) * invs, XMVectorGetY(cross) * invs, XMVectorGetZ(cross) * invs, s * 0.5f);
			q = XMQuaternionNormalize(q);
			rotMatrix = XMMatrixRotationQuaternion(q);
		}

		// 5. 부모 행렬에 회전 적용!
		parentMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // 이동 임시 제거
		parentMatrix = XMMatrixMultiply(parentMatrix, rotMatrix);
		parentMatrix.r[3] = parentPos; // 이동 복구
		XMStoreFloat4x4(&toRootTransforms[pIdx], parentMatrix); // 부모 완성!

		// =======================================================
		// 🌟 6. [진짜 제일 중요!!!] 뼈 다시 끼워 맞추기 (접골)
		// 회전된 최신 부모 행렬에 내 로컬 행렬을 곱해서 내 행렬도 갱신해 줍니다!
		// 이거 덕분에 다음 루프에서 내 자식이 날아가지 않아요!
		// =======================================================
		XMMATRIX finalChildRoot = XMMatrixMultiply(childLocal, parentMatrix);
		XMStoreFloat4x4(&toRootTransforms[cIdx], finalChildRoot);
	}
}