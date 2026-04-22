#pragma once
#include "Scene.h"
#include "MapGenerator/MapGenerator.h"

class CWorldItem;
class CItem;
class CMineableObject;

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
    void SetButtonEvents();
public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt);
    void Handle_S_MapEnd(std::shared_ptr<Session> session, const S_MapEnd& pkt);

    virtual void Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt) override;
    virtual void Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Item_List& pkt) override;
    virtual void Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt) override;
    virtual void Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt) override;
    virtual void Handle_S_RemoveItem(std::shared_ptr<Session> session, const S_RemoveItem& pkt) override;
    virtual void Handle_S_EquipItem(std::shared_ptr<Session>& session, const S_EquipItem& pkt) override;
    virtual void Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt) override;
    virtual void Handle_S_MineableList(std::shared_ptr<Session>& session, S_MineableList& pkt) override;
    virtual void Handle_S_DestroyMineable(std::shared_ptr<Session>& session, const S_DestroyMineable& pkt) override;
    virtual void Handle_S_UpdateDurability(std::shared_ptr<Session>& session, const S_UpdateDurability& pkt) override;

private:
    // 싱글용
    void SpawnWorldItem(uint16 itemID, XMFLOAT3 position);

    // 멀티용 (itemID는 도감번호, itemWorldId는 ObjectID)
    void SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position);

    // 플레이어 아이템 줍기
    void ProcessPickup();

    // 채굴 상호작용
    void ProcessMining();

    void DropItemAtPlayerFeet(std::shared_ptr<CItem> item);

private:
    std::vector<MapGenerator::InstanceData>  instance_data;
    std::vector<TreasureInfo>                treasures;
    std::vector<XMFLOAT3>                    humanMonster_spawn_positions;
    std::vector<XMFLOAT3>                    ghost_spawn_positions;

    static constexpr float  PICKUP_RANGE       = 2.0f;
    static constexpr float  MINING_RANGE       = 1.0f;
    static constexpr uint32 WORLD_ITEM_ID_BASE = 50000; // 플레이어/몬스터 ID 범위와 겹치지 않는 값
    uint32            world_item_id_counter = WORLD_ITEM_ID_BASE;

    bool              was_digging           = false;
    CMineableObject*  mining_target         = nullptr;
};

