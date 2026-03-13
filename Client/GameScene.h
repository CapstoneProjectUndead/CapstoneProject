#pragma once
#include "Scene.h"

class CGameScene : public CScene {
public:
    CGameScene();
    ~CGameScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;

    virtual void Enter() override;

    virtual void DrawUI() override;
    virtual bool IsUIInputEnabled() override;

private:

};

