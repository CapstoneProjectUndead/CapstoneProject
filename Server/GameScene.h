#pragma once
// ServerÂÊ GameScene
#include "Scene.h"
#include "GeometryLoader.h"

class CGameScene :
    public CScene
{
public:
    CGameScene(uint32 roomId);
    ~CGameScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

public:


private:
    void LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node);
    void LoadGameScene();
    void CreateGameScene();

private:
    std::map<std::string, std::shared_ptr<CObject>> prototypes;
};

