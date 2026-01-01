#include "pch.h"


int main()
{
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale(""));

    //shared_ptr<TcpServerService> serverService = std::make_shared<TcpServerService>(
    //    NetAddress(L"0.0.0.0", PORT_NUM),
    //    []() -> shared_ptr<Session> { return make_shared<ClientSession>(); },
    //    5);

    //ASSERT_CRASH(serverService->StartServer());

    //for (int i = 0; i < 5; ++i)
    //{
    //    GThreadManager->Launch([&serverService]() {
    //        serverService->GetIocpCore().WorkerThreadLoop();
    //        });
    //}

    GThreadManager->Join();
}
