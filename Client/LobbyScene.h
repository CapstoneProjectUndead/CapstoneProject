#pragma once
#include "Scene.h"

// 메인 UI 화면 상태 (한 번에 하나만 보임)
enum class LobbyUIState
{
    None,    // UI 없음 (인게임 등)
    Menu,    // ESC 누르면 옵션 메뉴 뜸.           
};

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene();
    ~CLobbyScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void Enter();
    virtual void Exit();

    virtual void DrawUI() override;
    virtual bool IsUIInputEnabled() override;

public:
    void SetUIState(LobbyUIState state) { ui_state = state; }
    LobbyUIState GetUIState() const { return ui_state; }

private:
    void DrawMenu();

private:
    LobbyUIState ui_state = LobbyUIState::None;
    LoadingType  loading_type = LoadingType::None;
    ActionResult pop_up_result;

    bool paused = false;
};

