#pragma once
// Server쪽 Lobbycene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene(uint32 roomId);
    virtual ~CLobbyScene() override;

    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void OnSceneActivate() override;
    virtual void OnSceneDeactivate() override;

public:
    void CheckReady();

public:
    //=================
    // 테스트용 함수
    void C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt);
    //=================

    virtual void Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt) override;
    void         Handle_C_Ready(shared_ptr<Session> session, const C_Ready& pkt);

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
};

