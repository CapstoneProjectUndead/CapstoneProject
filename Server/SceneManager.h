#pragma once
// ¼­¹öÂÊ SceneManager

#include "Scene.h"

class CTitleScene;
class CUser;

class CSceneManager
{
private:
    CSceneManager();
    CSceneManager(const CSceneManager&) = delete;

public:
    ~CSceneManager();

    static CSceneManager& GetInstance() {
        static CSceneManager instance;
        return instance;
    }

public:
    void    Initialize();
    void    Update(const float elapsedTime);
    void    SendResults();

#ifdef SCENE_TEST
    unique_ptr<CScene>* GetScenes() { return scenes; }
#endif

    CTitleScene* GetTitleScene() const { return title_scene.get(); }

private:
#ifdef SCENE_TEST
    unique_ptr<CScene>      scenes[(UINT)SCENE_TYPE::END];
#endif

    unique_ptr<CTitleScene> title_scene;
};

