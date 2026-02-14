#pragma once
// ServerÂÊ TestScene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene();
    ~CLobbyScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

public:
    void EnterPlayer(shared_ptr<Session> session, const C_LOGIN& pkt);
    void EnterLobby(shared_ptr<CUser> user, uint32 roomId);
};

