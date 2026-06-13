#pragma once
#include "Player.h"

struct DelayedPacket 
{
    // 패킷 데이터 (사용자님의 패킷 구조체나 바이트 배열)
    // 여기서는 예시로 상대방의 상태 데이터라고 가정합니다.
    OpponentFrameHistory data;

    // 이 패킷이 실제로 "처리되어야 할" 시간 (현재 시간 + 지연 시간)
    std::chrono::steady_clock::time_point releaseTime;
};

class CLagSimulator
{
public:
    // 지연 시간 설정 (밀리초 단위)
    void SetLatency(int ms) { latency_ms = ms; }
    void SetJitter(int ms) { jitter_ms = ms; } // 지터(변동폭) 추가 시

    // 패킷을 받으면 즉시 처리하지 않고 큐에 넣음
    void PushPacket(const OpponentFrameHistory& packetData);

    // 매 프레임 업데이트하며 시간이 된 패킷들을 반환
    void Update(std::vector<std::pair<OpponentFrameHistory, float>>& outPackets);

private:
    std::queue<DelayedPacket> delay_queue;
    int latency_ms = 0;
    int jitter_ms = 0;
};

