#pragma once
#include "Scene.h"
#include "MapGenerator/MapGenerator.h"

class CWorldItem;
class CItem;

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

    virtual void Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt) override;
    virtual void Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Item_List& pkt) override;
    virtual void Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt) override;
    virtual void Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt) override;

private:
    void DropItemAtPlayerFeet(std::shared_ptr<CItem> item);

    // 싱글용
    void SpawnWorldItem(uint16 itemID, XMFLOAT3 position);

    // 멀티용 (itemID는 도감번호, itemWorldId는 ObjectID)
    void SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position); 
    void ProcessPickup();

private:
    std::vector<MapGenerator::InstanceData>  instance_data;
    std::vector<TreasureInfo>                treasures;

    static constexpr float  PICKUP_RANGE       = 2.0f;
    static constexpr uint32 WORLD_ITEM_ID_BASE = 50000; // 플레이어/몬스터 ID 범위와 겹치지 않는 값
    uint32 world_item_id_counter = WORLD_ITEM_ID_BASE;
};

