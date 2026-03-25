#pragma once

#include "Scene.h"
#include "TitleScene.h"
#include "Renderers.h"

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
    void Init(ID3D12Device* device);
    void    Update();
    void    Render(ID3D12GraphicsCommandList* commandList);

    std::unique_ptr<CScene>* GetScenes() { return scenes; }

    CScene* GetActiveScene() const { return active_scene; }
    void    SetActiveScene(CScene* scene) { active_scene = scene; }

    CTitleScene* GetTitleScene() const 
    { 
        return static_cast<CTitleScene*>(scenes[(UINT)SCENE_TYPE::TITLE].get()); 
    }

    void ChangeScene(SCENE_TYPE type);

    auto& GetShaders() { return shaders; }
    void SetShaders(auto& otherShaders) { shaders = otherShaders; }

    auto& GetRanderers() { return renderers; }
    void SetRanderers(auto& otherShaders) { shaders = otherShaders; }
private:
    std::unordered_map<std::string, std::shared_ptr<CShader>>	shaders{};
    std::map<std::string, std::unique_ptr<IRenderer>> renderers;	// shader에 버퍼 설정하는 멤버 변수(rendering 담당)

    std::unique_ptr<CScene> scenes[(UINT)SCENE_TYPE::END];
    CScene*                 active_scene = nullptr;
};

