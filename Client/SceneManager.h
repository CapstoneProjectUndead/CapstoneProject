#pragma once

#include "Scene.h"

class CSceneManager
{
private:
    CSceneManager() = default;
    CSceneManager(const CSceneManager&) = delete;

public:
    ~CSceneManager() {};

    static CSceneManager& GetInstance() {
        static CSceneManager instance;
        return instance;
    }

public:
    void    Update();
    void    Render(ID3D12GraphicsCommandList* commandList);

    std::unique_ptr<CScene>* GetScenes() { return scenes; }

    CScene* GetActiveScene() const { return active_scene; }
    void    SetActiveScene(CScene* scene) { active_scene = scene; }

private:
    std::unique_ptr<CScene> scenes[(UINT)SCENE_TYPE::END];
    CScene*                 active_scene = nullptr;
};

