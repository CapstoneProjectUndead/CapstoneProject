#pragma once
#include "Object.h"
#include "SkinnedData.h"
#include "GeometryLoader.h"

// 생성 시 Initialize 호출
class CCharacter : public CObject
{
public:
    CCharacter();
    
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
    void Update(float deltaTime) override;
    void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList) override;
    void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
protected:
    std::unique_ptr<CAnimator> animator;
};