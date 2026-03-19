#pragma once
#include "Scene.h"

class CGameScene : public CScene 
{
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

public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt);

private:
    std::vector<TreasureInfo> treasures;
};

