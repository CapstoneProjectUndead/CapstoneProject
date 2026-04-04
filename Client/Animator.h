#pragma once
#include "Component.h"
struct AnimationData;

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent();

	void Play(const std::string& name);
	AnimationData GetAnimationData();

	void Update(float deltaTime) override;

	void UpdatePlayerAnimation();
	void UpdateMonsterAnimation();

	XMVECTOR GetHeadPosition();
private:
	std::string current_animation{ "Ganga_walk" };
	float current_time{};
};
