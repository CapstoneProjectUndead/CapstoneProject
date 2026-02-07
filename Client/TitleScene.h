#pragma once
#include "Scene.h"

class CTitleScene :
    public CScene
{
public:
    CTitleScene();
    ~CTitleScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Update(float elapsedTime) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;
};

