#pragma once
// Server쪽 GameScene
#include "Scene.h"
#include "GeometryLoader.h"
#include "MineableObject.h"
#include <MapGenerator/MapGenerator.h>

class CGameScene :
    public CScene
{
    friend class CLobbyScene;
public:
    CGameScene(uint32 roomId);
    virtual ~CGameScene() override;

    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void OnSceneActivate() override;
    virtual void OnSceneDeactivate() override;

    virtual void EnterScene(shared_ptr<CPlayer> player) override;
    virtual void LeaveScene(uint64 playerId) override;

private:
    void LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node);
    void LoadGameScene();
    void CreateGameScene();
    void UpdateMonsters(float elapsedTime);

public:
    virtual void Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt) override;
    virtual void Handle_C_Drop_Item(shared_ptr<Session> session, const C_DropItem& pkt) override;
    virtual void Handle_C_Equip_Item(shared_ptr<Session> session, const C_EquipItem& pkt) override;
    virtual void Handle_C_Use_Item(shared_ptr<Session> session, const C_UseItem& pkt) override;

    // 채굴 가능 오브젝트 관련
    static constexpr float MINING_RANGE = 1.0f;
    XMFLOAT3 FindSpawnPoint() const;
    CMineableObject* FindNearestMineable(const XMFLOAT3& pos, float range);
    void DestroyMineable(uint32 world_id);

    const vector<MonsterSpawnInfo>& GetMonsterSpawnInfo() const { return monster_spawn_info; }

private:
    map<string, shared_ptr<CObject>>                prototypes;
    vector<MapGenerator::InstanceData>              map_instance_data;
    vector<MonsterSpawnInfo>                        monster_spawn_info;

    map<uint64, shared_ptr<CMineableObject>>        mineable_objects;
    uint64                                          mineable_id_counter;
};

