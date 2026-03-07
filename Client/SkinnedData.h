#pragma once

struct SkeletonData;

// 
struct Keyframe
{
	Keyframe();

	float time_pos;
	XMFLOAT3 translation;
	XMFLOAT3 scale;
	XMFLOAT4 rotation;
};


struct DynamicBoneNode {
	XMFLOAT3 current_position{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
};

struct DynamicBoneChain {
	std::vector<DynamicBoneNode> nodes;
	std::vector<int> bone_indices;
	float bone_length = 1.0f;
};


struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

	void Interpolate(float t, XMFLOAT4X4& M)const;

	std::vector<Keyframe> key_frames;
};

struct AnimationClip
{
	float GetClipStartTime()const;
	float GetClipEndTime()const;

	void Interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms)const;

	std::vector<BoneAnimation> bone_animations;
};

class CSkinnedData
{
public:
	UINT BoneCount()const;

	float GetClipStartTime(const std::string& clipName)const;
	float GetClipEndTime(const std::string& clipName)const;

	void Set(const std::vector<int>& boneHierarchy, const std::vector<DirectX::XMFLOAT4X4>& boneOffsets, const std::unordered_map<std::string, AnimationClip>& animations);

	// 🌟 [수정됨] 끝부분에 elapsedTime이랑 목걸이 포인터 3개 추가! (기본값 nullptr 적용)
	void GetFinalTransforms(const std::string& clipName, float timePos, std::vector<DirectX::XMFLOAT4X4>& finalTransforms, const float pitch,
		float elapsedTime = 0.0f,
		DynamicBoneChain* leftEar = nullptr,
		DynamicBoneChain* rightEar = nullptr,
		DynamicBoneChain* tail = nullptr);

	AnimationClip& GetAnimation(const std::string& name) { return animations.at(name); }

public:
	// cached
	XMFLOAT3 head_position{};

private:
	// 🌟 [추가됨] CSkinnedData 안에서 구슬 흔들기를 처리할 도우미 함수 선언!
	void SimulateChain(DynamicBoneChain& chain,
		std::vector<DirectX::XMFLOAT4X4>& toRootTransforms,
		const std::vector<DirectX::XMFLOAT4X4>& toParentTransforms,
		float elapsedTime);

private:
	// 뼈대들의 부모 색인
	std::vector<int> bone_hierarchy;
	std::vector<DirectX::XMFLOAT4X4> bone_offsets;
	std::unordered_map<std::string, AnimationClip> animations;
};

struct SkinnedDataCB
{
    XMFLOAT4X4 bone_transforms[100];  // boneCount 이하
};