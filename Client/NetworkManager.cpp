#include "NetworkManager.h"
#include "ServerSession.h"
#include "ServerEngine/SocketHelper.h"

NetworkManager::NetworkManager()
{

}

NetworkManager::~NetworkManager()
{

}

void NetworkManager::ServiceStart()
{
	setlocale(LC_ALL, "");
	std::wcout.imbue(std::locale(""));

    SocketHelper::Init();

	client_service = std::make_shared<TcpClientService>(
       NetAddress(L"127.0.0.1", PORT_NUM),
       [this]()->shared_ptr<Session> { return make_shared<ServerSession>(); },
       1);

	ASSERT_CRASH(client_service->StartClientService());
}

void NetworkManager::Update()
{
    client_service->GetIocpCore().WorkerThreadLoop();
}