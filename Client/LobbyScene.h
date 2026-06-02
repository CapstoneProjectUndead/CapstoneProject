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
private:
    // C키 상호작용 구역
    enum class InteractZone { None, Reaper, Entrance };

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

    // 현재 플레이어가 속한 상호작용 구역(가까운 앵커, 반경 내) 반환
    InteractZone GetInteractZone() const;

    // 상호작용 구역 안일 때 "Press C key" 안내 표시
    void DrawInteractPrompt(InteractZone zone);

    // 상점 열림/닫힘 전환 시 인벤토리/커서 상태 처리
    void HandleShopTransition();

public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapStart(std::shared_ptr<Session> session, const S_MapStart& pkt);
    void Handle_S_Ready(std::shared_ptr<Session> session, const S_Ready& pkt);
    void Handle_S_RefreshStore(std::shared_ptr<Session> session, const S_RefreshStore& pkt);

private:
    XMFLOAT2 reaper_anchor    = { -2.228f, 3.414f };   // 사신(상점) NPC 앞
    XMFLOAT2 entrance_anchor  = { -0.102f, 4.955f };   // Ground 입구 앞
    static constexpr float interact_radius_sq = 1.0f * 1.0f;   // 상호작용 반경(제곱)

    // 상점 전환 추적
    bool shop_was_open       = false;
    bool prev_inventory_open = false;
};

