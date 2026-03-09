#pragma once
#include "Scene.h"

class CGameScene : public CScene {
public:
    CGameScene();
    ~CGameScene();

    void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    void Initialize() override;
    void Update(float elapsedTime) override;
    void Render(ID3D12GraphicsCommandList*) override;

    void Enter() override;

    void DrawUI() override;
    bool IsUIInputEnabled() override;

private:
};

