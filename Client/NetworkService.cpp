#include "NetworkService.h"

NetworkService::NetworkService()
{

}

NetworkService::~NetworkService()
{

}

void NetworkService::ServiceStart()
{
	//setlocale(LC_ALL, "");
   //std::wcout.imbue(std::locale(""));

   //client_service = std::make_unique<TcpClientService>(
   //    NetAddress(L"192.168.219.164", PORT_NUM),
   //    []()->Session* { return new ChattingServerSession; },
   //    1);

   //ASSERT_CRASH(clientService->StartClientService());

	// 아래는 수정 예정

   //for (int i = 0; i < 1; ++i)
   //{
   //    GThreadManager->Launch([clientService]() {
   //        clientService->GetIocpCore().WorkerThreadLoop();
   //        }
   //    );
   //}

   //GThreadManager->Join();
}