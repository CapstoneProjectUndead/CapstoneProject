#pragma once
#include "Scene.h"
#include <unordered_set>

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
    virtual bool IsMenuOpen() const override { return ui_state == LobbyUIState::Menu; }
public:
    void InteractWithReaper();
    void SetButtonEvents();

    // ESC 메뉴 (ImGui). paused일 때 씬 업데이트/입력 전송을 멈춰 일시정지처럼 동작.
    void DrawMenu();
    void DrawRoomLeavePopUp();

    // 준비 상태 패널(ImGui): 참가자 이름 + 준비 색박스(준비완료=초록/대기중=빨강). 멀티 전용.
    void DrawReadyPanel();

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

    // ESC 메뉴 상태
    LobbyUIState ui_state = LobbyUIState::None;
    bool         paused   = false;

    // 준비 완료한 타인 플레이어 ID (본인은 my_player->GetIsReady() 사용). Enter에서 clear.
    std::unordered_set<uint64> ready_player_ids;
};

