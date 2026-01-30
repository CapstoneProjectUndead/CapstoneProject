#pragma once
// Server쪽 TestScene
#include "Scene.h"

class CTestScene :
    public CScene
{
public:
    CTestScene();
    ~CTestScene();

    virtual void Update(float elapsedTime) override;

public:
    void EnterPlayer(shared_ptr<Session> session, const C_LOGIN& pkt);

    // 서버 권한 + 클라 예측 기반 Move
    void MovePlayer(shared_ptr<Session> session, const C_Input& pkt);
};

