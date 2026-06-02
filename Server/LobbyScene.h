#pragma once
// Server쪽 Lobbycene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
    // C키 상호작용 구역
    enum class InteractZone { None, Reaper, Entrance };

public:
    CLobbyScene(uint32 roomId);
    virtual ~CLobbyScene() override;

    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void OnSceneActivate() override;
    virtual void OnSceneDeactivate() override;

public:
    void CheckReady();

    virtual void EnterScene(shared_ptr<CPlayer> player) override;

public:
    //=================
    // 테스트용 함수
    void C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt);
    //=================

    virtual void Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt) override;
    void         Handle_C_Ready(shared_ptr<Session> session, const C_Ready& pkt);
    void         Handle_C_ShopState(shared_ptr<Session> session, const C_ShopState& pkt);
    void         Handle_C_BuyItem(shared_ptr<Session> session, const C_BuyItem& pkt);
    void         Handle_C_SpendCoin(shared_ptr<Session> session, const C_SpendCoin& pkt);
    void         Handle_C_RefreshStore(shared_ptr<Session> session, const C_RefreshStore& pkt);

private:
    enum class LobbyMeshName {
        Wall,
        Floor,
        GroundPipe,
        Unknown
    };

    LobbyMeshName stringToLobbyMeshName(const std::string& str);

    void CreateLobby();
    void SendPlayerToGameScene();

private:
    int  player_ready_cnt;

    XMFLOAT2 reaper_anchor = { -2.228f, 3.414f };   // 사신(상점) NPC 앞
    static constexpr float interact_radius_sq = 1.0f * 1.0f;   // 상호작용 반경(제곱)
};

