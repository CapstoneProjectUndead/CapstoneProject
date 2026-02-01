#pragma once
#include "Timer.h"
#include "ServerPacketHandler.h"

// Client-Server Clock Synchronization
// 서버와 시간을 동기화하기 위해 만든 클래스

class CNetworkClockManager
{
private:
    CNetworkClockManager() = default;
    CNetworkClockManager(const CNetworkClockManager&) = delete;

public:
    ~CNetworkClockManager() {};

    static CNetworkClockManager& GetInstance() {
        static CNetworkClockManager instance;
        return instance;
    }

public:
    void SendPing(std::shared_ptr<Session> session)
    {
        C_Ping pingPkt;
        pingPkt.clientTime = GetClientNow();
        auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Ping>(pingPkt);
        session->DoSend(sendBuffer);
    }

    void UpdateClockSync(float clientSendTime, float serverTime, float clientRecvTime)
    {
        float rtt = clientRecvTime - clientSendTime;
        float estimatedServerNow = serverTime + rtt * 0.5f;

        float newOffset = estimatedServerNow - clientRecvTime;
        clock_offset = Lerp(clock_offset, newOffset, 0.1f);
    }

    float GetClientNow()
    {
        return g_client_total_time;
    }

    float GetServerNow()
    {
        return GetClientNow() + clock_offset;
    }

    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

private:
    double clock_offset = 0.0f;
};
