#pragma once
#include "Player.h"


class CMyPlayer :
    public CPlayer
{
public:
    using CObject::Move; // 부모의 Move 모두 노출

    CMyPlayer();
    ~CMyPlayer() {};

    virtual void Update(float elapsedTime) override;

    void ProcessInput();

    std::weak_ptr<Session>   GetSessionWeak() const { return session; }
    std::shared_ptr<Session> GetSession() const { return session.lock(); }
    void SetSession(std::shared_ptr<Session> _session) { session = _session; }

private:
    std::weak_ptr<Session> session;

    const float move_packet_send_dely = 0.2f;
    float move_packet_send_timer = move_packet_send_dely;
};

