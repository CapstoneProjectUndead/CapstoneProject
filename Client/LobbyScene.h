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

    void StartLoading(LoadingType type) { loading_type = type; }
    void StopLoading() { loading_type = LoadingType::None; }

    void InteractWithReaper();
    void SetupDialogueEvents();
    // 플레이어가 준비되었는지 체크 후 UI 변경
    void UpdatePlayerReadyUI();
private:
    void DrawMenu();
    void DrawRoomLeavePopUp();
    void DrawLoadingPopUp();

public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapStart(std::shared_ptr<Session> session, const S_MapStart& pkt);

private:
    LobbyUIState ui_state = LobbyUIState::None;
    LoadingType  loading_type = LoadingType::None;
    ActionResult pop_up_result;

    bool paused = false;
};

