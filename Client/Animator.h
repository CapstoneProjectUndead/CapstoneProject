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

	XMFLOAT3 GetHeadPosition() const { return *head_position; }
private:
	const XMFLOAT3*const head_position;
	std::string current_animation{ "Ganga_walk" };
	float current_time{};
};
