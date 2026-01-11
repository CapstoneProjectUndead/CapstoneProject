#include "stdafx.h"
#include "NetworkManager.h"
#include "Core.h"

#include <ServerEngine/global.h>
#include <ServerEngine/ThreadManager.h>
#include <ServerEngine/Service.h>
#include "ServerSession.h"
#include "ServerEngine/SocketHelper.h"
#include "ServerPacketHandler.h"

#include <protocol.h>

CNetworkManager::CNetworkManager()
{

}

CNetworkManager::~CNetworkManager()
{

}

void CNetworkManager::ServiceStart()
{
	setlocale(LC_ALL, "");
	std::wcout.imbue(std::locale(""));

    SocketHelper::Init();

    // 서버로부터 받은 패킷을 처리하는 
    // 헬퍼 클래스를 초기화
    CServerPacketHandler::Init();

	client_service = std::make_shared<TcpClientService>(
       NetAddress(L"127.0.0.1", PORT_NUM),
       [this]()->std::shared_ptr<Session> { return std::make_shared<CServerSession>(); },
       1);

	ASSERT_CRASH(client_service->StartClientService());
}

void CNetworkManager::Update()
{
    client_service->GetIocpCore().WorkerThreadLoop();
}