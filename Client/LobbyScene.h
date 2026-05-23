#pragma once
#include "Scene.h"

enum class LobbyUIState
{
    None,    // UI 
    Menu,    // ESC          
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
    void InteractWithReaper();
    void SetButtonEvents();
    // 플레이어가 준비되었는지 체크 후 UI 변경
    void UpdatePlayerReadyUI();
public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapStart(std::shared_ptr<Session> session, const S_MapStart& pkt);
    void Handle_S_Ready(std::shared_ptr<Session> session, const S_Ready& pkt);
};

