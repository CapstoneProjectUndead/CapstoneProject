#include "pch.h"
#include "ClientSession.h"
#include "SceneManager.h"

int main()
{
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale(""));

    //SocketHelper::Init();

    // 클라이언트로부터 받은 패킷을 처리하는 
    // 헬퍼 클래스를 초기화
    CClientPacketHandler::Init();

    // Scene 초기화
    CSceneManager::GetInstance().Initialize();

    shared_ptr<TcpServerService> serverService = std::make_shared<TcpServerService>(
        NetAddress(L"127.0.0.1", PORT_NUM),
        []() -> shared_ptr<Session> { return make_shared<CClientSession>(); },
        5);

    ASSERT_CRASH(serverService->StartServer());

    for (int i = 0; i < 5; ++i)
    {
        GThreadManager->Launch([&serverService]() {
            serverService->GetIocpCore().WorkerThreadLoop();
            });
    }

    GThreadManager->Join();
}
