#pragma once
#include "Player.h"

struct ClientFrameHistory 
{
    uint64       seq_num;
    float        duration;
    InputData    input;
    XMFLOAT3     predictedPos; // 내가 예측했던 결과 좌표
    PLAYER_STATE state;
};


class CMyPlayer :
    public CPlayer
{
public:
    CMyPlayer();
    ~CMyPlayer() {};

    virtual void Update(float elapsedTime) override;

    std::weak_ptr<Session>   GetSessionWeak() const { return session; }
    std::shared_ptr<Session> GetSession() const { return session.lock(); }
    void SetSession(std::shared_ptr<Session> _session) { session = _session; }

    // 클라이언트 예측을 서버 기준에 맞게 다시 보정하는 코드
    void ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos);

private:
    void ProcessRotation();
    void ProcessInput();

    // 서버 권위 방식 + 클라 예측 이동 방식
    void ServerAuthorityMove(const float elapsedTime);
    void CaptureInput(InputData& currentInput);
    void PredictMove(const InputData& input, float dt);

private:
    std::weak_ptr<Session> session;

    const float move_packet_send_delay = 1.0f / 60.0f;
    float move_packet_send_timer = move_packet_send_delay;

    // 프레임 시간을 누적시키기 위해서 추가한 변수
    float dt_accumulator = 0.0f;

    // 클라 예측 이동을 위한 시퀀스 넘버
    uint64                            client_seq_counter = 0;
    std::deque<ClientFrameHistory>    history_deq; // 시퀀스 장부
};

