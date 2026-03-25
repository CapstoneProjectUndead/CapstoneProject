#pragma once
#include "Scene.h"
#include "MapGenerator/MapGenerator.h"

class CGameScene : public CScene 
{
public:
    CGameScene();
    ~CGameScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void Enter() override;
    virtual void Exit() override;

    virtual void DrawUI() override;
    virtual bool IsUIInputEnabled() override;

public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt);
    void Handle_S_MapEnd(std::shared_ptr<Session> session, const S_MapEnd& pkt);

private:
    std::vector<MapGenerator::InstanceData> instance_data;
    std::vector<TreasureInfo> treasures;
};

