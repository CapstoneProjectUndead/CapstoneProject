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
};

