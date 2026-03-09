#pragma once
#include "Scene.h"

// ���� UI ȭ�� ���� (�� ���� �ϳ��� ����)
enum class LobbyUIState
{
    None,    // UI ���� (�ΰ��� ��)
    Menu,    // ESC ������ �ɼ� �޴� ��.           
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
    void DrawRoomLeavePopUp();

private:
    LobbyUIState ui_state = LobbyUIState::None;
    LoadingType  loading_type = LoadingType::None;
    ActionResult pop_up_result;

    bool paused = false;
};

