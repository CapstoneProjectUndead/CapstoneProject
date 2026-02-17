#pragma once
// Server쪽 TestScene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene();
    CLobbyScene(uint32 roomId);
    ~CLobbyScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

public:
    //=================
    // 테스트용 함수
    void C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt);
    //=================

    void C_Enter_Lobby(shared_ptr<Session> session, const PktDummy& pkt);
};

