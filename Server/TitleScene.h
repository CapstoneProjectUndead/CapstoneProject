#pragma once
// ¼­¹öÂÊ TitleScene
#include "Scene.h"


class CTitleScene :
    public CScene
{
public:
    CTitleScene();
    ~CTitleScene();

    virtual void Update(float elapsedTime) override;

public:
    void HandleSignUp(shared_ptr<Session> session, const C_SIGNUP& pkt);
    void HandleLogIn(shared_ptr<Session> session, const C_LOGIN& pkt);
    void HandleLogOut(shared_ptr<Session> session, const C_LOGOUT& pkt);
};

