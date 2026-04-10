#pragma once
// Server�� CustomScene
#include "Scene.h"


class CCustomScene :
    public CScene
{
public:
    CCustomScene(uint32 roomId);
    ~CCustomScene();

    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void OnSceneActivate() override;
    virtual void OnSceneDeactivate() override;

public:
    void C_Handle_Enter_CustomScene(shared_ptr<Session> session, const C_EnterRoom& pkt);
    void C_Handle_Custom_Select(shared_ptr<Session> session, const C_CustomSelect& pkt);

private:
    int error = 1;
};

