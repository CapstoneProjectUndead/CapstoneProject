#pragma once
// ¼­¹öÂÊ SceneManager

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
    void    Initialize();
    void    Update(float elapsedTime);

    unique_ptr<CScene>* GetScenes() { return scenes; }

private:
    unique_ptr<CScene>      scenes[(UINT)SCENE_TYPE::END];
};

