#pragma once
#include "Player.h"

struct FrameHistory 
{
    uint64_t seq_num;
    float deltaTime;
    InputData input;
    XMFLOAT3 predictedPos; // 내가 예측했던 결과 좌표
};


class CMyPlayer :
    public CPlayer
{
public:
    using CObject::Move; // 부모의 Move 모두 노출

    CMyPlayer();
    ~CMyPlayer() {};

    virtual void Update(float elapsedTime) override;

    std::weak_ptr<Session>   GetSessionWeak() const { return session; }
    std::shared_ptr<Session> GetSession() const { return session.lock(); }
    void SetSession(std::shared_ptr<Session> _session) { session = _session; }

private:
    void ProcessInput();

    // 기존 클라 권위 방식 Move + Rotate
    void ClientAuthorityMove(float elapsedTime);

    void ProcessRotation();
    void PredictMove(const InputData& input, float dt);

    void ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos);

private:
    std::weak_ptr<Session> session;

    const float move_packet_send_dely = 0.2f;
    float move_packet_send_timer = move_packet_send_dely;

    // 클라 예측 이동을 위한 시퀀스 넘버
    uint64_t                    client_seq_counter = 0;
    std::deque<FrameHistory>    history_deque; // 시퀀스 장부
};

