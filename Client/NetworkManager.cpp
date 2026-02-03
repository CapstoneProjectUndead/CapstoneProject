#include "stdafx.h"
#include "NetworkManager.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "LagSimulator.h"
#include "SceneManager.h"
#include "NetworkClockManager.h"


CNetworkManager::CNetworkManager()
{

}

CNetworkManager::~CNetworkManager()
{

}

void CNetworkManager::ServiceStart(std::wstring address, uint16 port)
{
	setlocale(LC_ALL, "");
	std::wcout.imbue(std::locale(""));

    SocketHelper::Init();

    jitter_measurer = std::make_unique<CJitterMeasurer>();
   
#ifdef GENERATE_LAG
    lag_simulator = std::make_unique<CLagSimulator>();
    lag_simulator->SetLatency(1000);
    lag_simulator->SetJitter(500);
#endif 

    // 서버로부터 받은 패킷을 처리하는 
    // 헬퍼 클래스를 초기화
    CServerPacketHandler::Init();

	client_service = std::make_shared<TcpClientService>(
       NetAddress(address, port),
       [this]()->std::shared_ptr<Session> { return std::make_shared<CServerSession>(); },
       1);

	ASSERT_CRASH(client_service->StartClientService());
    Tick(0);
}

void CNetworkManager::Tick(float time)
{
    client_service->GetIocpCore().WorkerThreadLoop(time);

#ifdef GENERATE_LAG
    UpdateDelayPacket();
#endif 
}

void CNetworkManager::OnRecvOpponentPos(const OpponentFrameHistory& packet)
{
    lag_simulator->PushPacket(packet);
}

void CNetworkManager::UpdateDelayPacket()
{
    // pair.first: 패킷 데이터, pair.second: 논리적 방출 시간(float)
    std::vector<std::pair<OpponentFrameHistory, float>> readyPackets;
    lag_simulator->Update(readyPackets);

    for (auto& pair : readyPackets) {
        // 1. 시뮬레이터가 계산해준 "방출 시간"으로 지터 측정
        // 이렇게 해야 측정기가 200ms+지터의 렉을 감지합니다.
        CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(pair.second);

        // 2. 해당 플레이어를 찾아 장부에 데이터 삽입
        auto scene = CSceneManager::GetInstance().GetActiveScene();
        if (scene) {
            auto& indexMap = scene->GetIDIndex();
            auto& vec = scene->GetObjects();

            auto it = indexMap.find(pair.first.player_id);
            if (it != indexMap.end()) {

                size_t idx = it->second;
                if (idx >= vec.size())
                    return;

                auto otherPlayer = std::static_pointer_cast<CPlayer>(vec[idx]);
                if (otherPlayer) {
                    otherPlayer->RecordOpponentFrameHistory(pair.first);
                }
            }
        }
    }
}
