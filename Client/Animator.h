#pragma once
#include "Component.h"
#include "AnimationController.h"
struct AnimationData;

struct AnimLayer {
	std::string current_clip;
	float weight{ 1.0f };
	// animationManager.bone_masks의 인덱스와 일치시키기(-1이면 전체 적용)
	int mask_id{ -1 };
	float start_time{};
};

// 캐릭터별 애니메이션 셋
struct CharacterAnimSet {
	std::string idle;
	std::string walk;
	std::string run;
	std::string action; // 상반신용 기본 액션
};

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent();
	void CAnimatorComponent::Init(const CharacterAnimSet& animSet);
	void CAnimatorComponent::AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run);

	// layer 1
	void PlayAction(const std::string& clipName);
	AnimationData GetAnimationData();

	void Update(float deltaTime) override;
	void UpdateLayerWeights(float deltaTime);

	void UpdatePlayerAnimation();
	void UpdateMonsterAnimation();

	XMVECTOR GetHeadPosition();
private:
	std::vector<AnimLayer> layers;
	CAnimationController controller;
	float current_time{};
	std::string current_animation{ "Ganga_walk" };
	CharacterAnimSet anim_set;
};
