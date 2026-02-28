#pragma once
#include "Scene.h"

class CGameScene : public CScene {
public:
    CGameScene();
    ~CGameScene();

    void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    void Initialize() override;
    void Update(float elapsedTime) override;

    void Enter() override;
    void Exit() override;

    void DrawUI() override;
    bool IsUIInputEnabled() override;

private:
    // GameScene에 있는 prototype 저장
    std::map<std::string, std::shared_ptr<CObject>> prototypes;
};

