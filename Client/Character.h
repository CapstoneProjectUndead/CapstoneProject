#pragma once
#include "Object.h"
#include "SkinnedData.h"
#include "GeometryLoader.h"

struct SkinnedDataCB
{
    XMFLOAT4X4 boneTransforms[100];  // boneCount 이하
};

class CCharacter : public CObject
{
public:
    CCharacter();
    
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    void Update(float deltaTime) override;
    void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList) override;
    void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
protected:
    std::string current_animation{"Ganga_idle"};
    CSkinnedData skinned;
    std::vector<XMFLOAT4X4> final_transforms;
    ComPtr<ID3D12Resource> skinned_cb;
    float current_time{};
};