#pragma once
#include "Object.h"
#include "SkinnedData.h"

// 생성 시 Initialize 호출
class CCharacter : public CObject
{
public:
    CCharacter();
    
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
};