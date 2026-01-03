#include "NetworkService.h"
#include "ServerSession.h"
#include "ServerEngine/SocketHelper.h"

NetworkService::NetworkService()
{

}

NetworkService::~NetworkService()
{

}

void NetworkService::ServiceStart()
{
	setlocale(LC_ALL, "");
	std::wcout.imbue(std::locale(""));

    SocketHelper::Init();

	client_service = std::make_shared<TcpClientService>(
       NetAddress(L"127.0.0.1", PORT_NUM),
       [this]()->shared_ptr<Session> { return make_shared<ServerSession>(); },
       1);

	ASSERT_CRASH(client_service->StartClientService());

	// 아래는 수정 예정

    //for (int i = 0; i < 1; ++i)
    //{
    //    GThreadManager->Launch([this]() {
    //        client_service->GetIocpCore().WorkerThreadLoop();
    //        }
    //    );
    //}
    
    //GThreadManager->Join();
}