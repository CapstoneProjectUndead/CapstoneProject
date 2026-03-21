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
    ~CGameScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

private:
    void LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node);
    void LoadGameScene();
    void CreateGameScene();

private:
    std::map<std::string, std::shared_ptr<CObject>> prototypes;
    vector<MapGenerator::InstanceData>              map_instance_data;
    std::vector<TreasureInfo>                       treasures;

    std::vector<std::string> GameSceneTypeToString(const MapGenerator::EModelType& type);
    std::string              GetVariantFileName(EModelVariant variant);
    std::string              PickRandom(const std::string& key);
    EModelVariant            PickRandomVariant(const std::string& key);
};

