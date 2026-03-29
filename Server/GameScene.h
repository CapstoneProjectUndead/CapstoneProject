#pragma once
// Server쪽 GameScene
#include "Scene.h"
#include "GeometryLoader.h"
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

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node);
    void LoadGameScene();
    void CreateGameScene();

public:
    virtual void Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt) override;
    virtual void Handle_C_Drop_Item(shared_ptr<Session> session, const C_DropItem& pkt) override;

private:
    map<string, shared_ptr<CObject>>    prototypes;
    vector<MapGenerator::InstanceData>  map_instance_data;
};

