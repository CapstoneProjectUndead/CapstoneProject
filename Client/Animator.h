#pragma once
#include "Component.h"
#include "SkinnedData.h"

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent();
	void Initialize(const std::string& charName, const std::string& AniName);

	void Play(const std::string& name);

	void Update(float deltaTime) override;

	void UpdatePlayerAnimation();
	void UpdateMonsterAnimation();

	const std::vector<XMFLOAT4X4>& GetFinalTransforms() const { return final_transforms; }
	XMFLOAT3 GetHeadPosition() const { return *head_position; }
private:
	CSkinnedData skinned;
	const XMFLOAT3*const head_position;
	std::string current_animation{ "Ganga_idle" };
	float current_time{};
	std::vector<XMFLOAT4X4> final_transforms;
};
