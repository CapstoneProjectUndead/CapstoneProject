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
    vector<MapGenerator::InstanceData> instance_data;

    // MapGenerator로 생성되는 grid를 model과 매치
    std::vector<std::string> GameSceneTypeToString(const MapGenerator::EModelType& type);
    std::string PickRandom(const std::string& key);
};

