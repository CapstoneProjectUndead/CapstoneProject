#include "pch.h"
#include "ClientSession.h"
#include "GameFramework.h"
#include "TimeManager.h"


unique_ptr<class CGameFramework> gGameFramework;

const double g_server_targetTick = 60.0; // 60Hz
const double g_targetDT = 1.0 / g_server_targetTick; // 0.01666... (16.6ms)


int main()
{
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale(""));

    // 클라이언트로부터 받은 패킷을 처리하는 
    // 헬퍼 클래스를 초기화
    CClientPacketHandler::Init();

    // 타이머 초기화
    CTimeManager::GetInstance().Init();

    // GameFramework 초기화
    gGameFramework = std::make_unique<CGameFramework>();
    gGameFramework->Init();

    shared_ptr<TcpServerService> serverService = std::make_shared<TcpServerService>(
        NetAddress(L"127.0.0.1", PORT_NUM),
        []() -> shared_ptr<Session> { return make_shared<CClientSession>(); },
        5);

    ASSERT_CRASH(serverService->StartServer());

    // 네트워크 패킷을 받는 워커 스레드 5개 배치
    for (int i = 0; i < 5; ++i)
    {
        GThreadManager->Launch([&serverService]() {
            serverService->GetIocpCore().WorkerThreadLoop();
            });
    }

    double accumulator = 0.0;


    while (true)
    {
        // 시간 계산 (delta_time)
        CTimeManager::GetInstance().Update();

        // 너무 큰 deltaTime 방지 (디버깅 등으로 멈췄을 때 갑자기 수백 번 업데이트 방지)
        double deltaTime = CTimeManager::GetInstance().GetClampedDeltaTime();

        accumulator += deltaTime;

        bool ticked = false;

        // 쌓인 시간만큼 "고정된 16.6ms"씩 업데이트를 돌림
        while (accumulator >= g_targetDT)
        {
            // 물리 및 충돌 업데이트 (서버 권위 판정)
            gGameFramework->Update(static_cast<float>(g_targetDT));

            accumulator -= g_targetDT;
            ticked = true;
        }

        // 결과를 클라들에게 브로드캐스트
        if (ticked) {

            // 가짜 렉 시뮬레이션 (C++11 random 사용)
            static std::default_random_engine generator;
            // 0ms ~ 50ms 사이의 랜덤 지터를 발생시킴
            std::uniform_real_distribution<float> distribution(0.0f, 0.05f);
            float fakeJitter = distribution(generator);

            // 실제로는 별도의 큐에 담아 스레드가 지연 후 전송해야 하지만, 
            // 간단한 테스트를 위해 sleep을 사용하거나(비권장), 
            // 여기서는 개념적으로 '지연 전송'을 시뮬레이션합니다.

            // 테스트용: 강제로 현재 스레드를 랜덤하게 쉬게 함 (서버 성능은 떨어지지만 클라 지터 테스트엔 직빵)
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(fakeJitter * 1000)));

            gGameFramework->SendResults();
        }

        // CPU 점유율 최적화: 아주 짧게 쉬어줌
        // 남은 시간이 아주 많을 때만 제한적으로 사용하거나 생략 가능
        if (accumulator < g_targetDT) {
            std::this_thread::yield();
        }
    }

    GThreadManager->Join();
}
