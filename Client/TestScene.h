#pragma once
#include "Scene.h"

class CTestScene :
    public CScene
{
public:
    CTestScene();
    ~CTestScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Update(float elapsedTime) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;
};

