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

	void GetFinalTransforms(const std::string& clipName, float timePos, std::vector<DirectX::XMFLOAT4X4>& finalTransforms)const;
	AnimationClip& GetAnimation(const std::string& name) { return animations.at(name); }
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