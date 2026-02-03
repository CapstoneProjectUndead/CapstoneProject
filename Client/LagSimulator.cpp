#include "stdafx.h"
#include "LagSimulator.h"
#include "NetworkClockManager.h"

void CLagSimulator::PushPacket(const OpponentFrameHistory& packetData)
{
    auto now = std::chrono::steady_clock::now();

    // 지연 시간 계산 (기본 핑 + 랜덤 지터)
    int finalDelay = latency_ms;
    if (jitter_ms > 0) {
        finalDelay += (rand() % jitter_ms);
    }

    DelayedPacket dp;
    dp.data = packetData;
    dp.releaseTime = now + std::chrono::milliseconds(finalDelay);

    delay_queue.push(dp);
}

void CLagSimulator::Update(std::vector<std::pair<OpponentFrameHistory, float>>& outPackets)
{
    auto now_chrono = std::chrono::steady_clock::now();

    // 현재 클라이언트의 서버 동기화 시간
    float clientServerNow = CNetworkClockManager::GetInstance().GetServerNow();

    while (!delay_queue.empty()) {
        auto& front = delay_queue.front();

        // 시간이 된 패킷들만 처리
        if (now_chrono >= front.releaseTime) {
            // [핵심] 이 패킷이 실제로 방출되어야 했던 "논리적 시점"을 계산
            // 현재 시간에서 (지금 - 방출예정시간)을 빼서 정확한 방출 시점을 복구합니다.
            auto delayDiff = std::chrono::duration_cast<std::chrono::microseconds>(now_chrono - front.releaseTime).count() / 1000000.0f;
            float logicReleaseTime = clientServerNow - delayDiff;

            outPackets.push_back({ front.data, logicReleaseTime });
            delay_queue.pop();
        }
        else {
            break;
        }
    }
}
