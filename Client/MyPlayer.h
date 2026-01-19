#pragma once
#include "Player.h"


class CMyPlayer :
    public CPlayer
{
public:
    using CObject::Move; // 부모의 Move 모두 노출

    CMyPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    ~CMyPlayer();

    virtual void Update(float elapsedTime) override;

    void ProcessInput();

    CCamera* GetCameraPtr() const { return camera.get(); }

    std::weak_ptr<Session>   GetSessionWeak() const { return session; }
    std::shared_ptr<Session> GetSession() const { return session.lock(); }
    void SetSession(std::shared_ptr<Session> _session) { session = _session; }

protected:
    std::shared_ptr<CCamera> camera;

private:
    std::weak_ptr<Session> session;

    const float move_packet_send_dely = 0.2f;
    float move_packet_send_timer = move_packet_send_dely;
};

