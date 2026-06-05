#pragma once

#include "Scene.h"
#include "TitleScene.h"
#include "Renderers.h"
#include "GBufferTarget.h"

class CShadowMap;
class CCubeShadowMap;
class CSkyBox;
class CRenderTarget;

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

    auto& GetCubeShadowMap() { return cube_shadow_map; }
    auto& GetShadowMap() { return dir_shadow_map; }
    auto& GetScreenShadowMap() { return screen_shadow_map; }
    auto& GetSkybox() { return skybox; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferColorRTV() const { return buffer_color->GetRTV(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferNormalRTV() const { return buffer_normal->GetRTV(); }
    ID3D12Resource* GetGBufferColorResource() const { return buffer_color->GetResource(); }
    ID3D12Resource* GetGBufferNormalResource() const { return buffer_normal->GetResource(); }
    // onResize 시 srv 삭제 됨(Gameframework에서 실행)
    void CreateMainDepthSRV(ID3D12Device* device);
private:
    void CreateMainDepthSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle);
private:
    std::vector<std::shared_ptr<CShader>>	shaders;
    std::vector<std::unique_ptr<IRenderer>> renderers;	// shader에 버퍼 설정하는 멤버 변수(rendering 담당)
    std::shared_ptr<CShadowMap> dir_shadow_map;
    std::shared_ptr<CRenderTarget> screen_shadow_map;
    std::shared_ptr<CCubeShadowMap> cube_shadow_map;

    std::shared_ptr<CSkyBox> skybox;
    std::unique_ptr<CGBufferTarget> buffer_color{};
    std::unique_ptr<CGBufferTarget> buffer_normal{};

    D3D12_CPU_DESCRIPTOR_HANDLE buffer_color_rtv_handle{};
    D3D12_CPU_DESCRIPTOR_HANDLE buffer_normal_rtv_handle{};

    std::unique_ptr<CScene> scenes[(UINT)SCENE_TYPE::END];
    CScene*                 active_scene = nullptr;
};

