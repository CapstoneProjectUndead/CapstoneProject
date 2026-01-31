#pragma once
#include "Component.h"
#include "SkinnedData.h"

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent() = default;
	void Initialize(const std::string& charName, const std::string& AniName);

	void Play(const std::string& name);

	void Update(float deltaTime) override;

	const std::vector<XMFLOAT4X4>& GetFinalTransforms() const { return final_transforms; }

	void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
private:
	CSkinnedData skinned;
	std::string current_animation{ "Ganga_idle" };
	float current_time{};
	std::vector<XMFLOAT4X4> final_transforms;

	ComPtr<ID3D12Resource> skinned_cb;
};
