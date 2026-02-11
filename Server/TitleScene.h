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

    void EnterUser(shared_ptr<CUser> user);
    void LeaveUser(uint64 id);

    const map<uint64, shared_ptr<CUser>>& GetUsers() const { return users; }

public:
    void HandleSignUp(shared_ptr<Session> session, const C_SIGNUP& pkt);
    void HandleLogIn(shared_ptr<Session> session, const C_LOGIN& pkt);
    void HandleLogOut(shared_ptr<Session> session, const C_LOGOUT& pkt);

private:
    mutex								users_lock;
    map<uint64, shared_ptr<CUser>>		users;
};

