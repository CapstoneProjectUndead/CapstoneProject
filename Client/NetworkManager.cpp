#include "stdafx.h"
#include "NetworkManager.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"


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

    // 서버로부터 받은 패킷을 처리하는 
    // 헬퍼 클래스를 초기화
    CServerPacketHandler::Init();

	client_service = std::make_shared<TcpClientService>(
       NetAddress(address, port),
       [this]()->std::shared_ptr<Session> { return std::make_shared<CServerSession>(); },
       1);

	ASSERT_CRASH(client_service->StartClientService());
}

void CNetworkManager::Tick(float time)
{
    client_service->GetIocpCore().WorkerThreadLoop(time);
}