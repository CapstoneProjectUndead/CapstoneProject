#pragma once
// Server쪽 TestScene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene();
    ~CLobbyScene();

    virtual void Update(float elapsedTime) override;

public:
    void EnterPlayer(shared_ptr<Session> session, const C_LOGIN& pkt);
    void EnterLobby(shared_ptr<CUser> user);

    // 서버 권한 + 클라 예측 기반 Move
    void MovePlayer(shared_ptr<Session> session, const C_Input& pkt);
};

