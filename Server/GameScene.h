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
    void AnnounceReturnPosition();
    void DetectPlayerReturns();
    void TriggerSettlement();
    void CheckEarlySettlement();

public:
    virtual void Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt) override;
    virtual void Handle_C_Drop_Item(shared_ptr<Session> session, const C_DropItem& pkt) override;
    virtual void Handle_C_Equip_Item(shared_ptr<Session> session, const C_EquipItem& pkt) override;
    virtual void Handle_C_Use_Item(shared_ptr<Session> session, const C_UseItem& pkt) override;
    void Handle_C_GiveUpRescue(shared_ptr<Session> session, const C_GiveUpRescue& pkt);

    // 라운드 타이머 (정산 시스템)
    static constexpr float ROUND_DURATION = 600.f;        
    static constexpr float RETURN_ACTIVATE_REMAIN = 120.f; // 종료 60초 전 복귀존 활성화
    static constexpr float RETURN_RANGE = 1.0f;           // 복귀존 반경

    float GetRoundTimer() const { return round_timer; }
    bool  IsRoundEnded() const { return round_ended; }

    // S_PlayerMove에 실어 보낼 동기화 값 (Scene::GetSyncedRoundTimer override)
    virtual float GetSyncedRoundTimer() const override { return round_started ? round_timer : -1.f; }

    // 채굴 가능 오브젝트 관련
    static constexpr float MINING_RANGE = 1.f;
    static constexpr float BARE_HAND_MINING_RANGE = 1.f;
    XMFLOAT3 FindSpawnPoint() const;
    CMineableObject* FindNearestMineable(const XMFLOAT3& pos, float range, MINEABLEOBJECT_TYPE type);
    void DestroyMineable(uint32 world_id);

    const vector<MonsterSpawnInfo>& GetMonsterSpawnInfo() const { return monster_spawn_info; }

private:
    map<string, shared_ptr<CObject>>                prototypes;
    vector<MapGenerator::InstanceData>              map_instance_data;
    vector<MonsterSpawnInfo>                        monster_spawn_info;

    map<uint64, shared_ptr<CMineableObject>>        mineable_objects;
    uint64                                          mineable_id_counter;

    // 라운드 타이머 상태
    float                                           round_timer;
    bool                                            round_started;
    bool                                            round_ended;

    // 복귀존 상태 (60초 시점 1회만 활성화 송신)
    bool                                            return_active;
    XMFLOAT3                                        return_center{};
};

