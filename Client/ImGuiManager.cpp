#include "stdafx.h"
#include "ImGuiManager.h"

CImGuiManager::CImGuiManager()
{

}

CImGuiManager::~CImGuiManager()
{
    Shutdown();
}

void CImGuiManager::Init(HWND hwnd, ID3D12Device* device, int numFramesInFlight, DXGI_FORMAT rtvFormat)
{
    // 1. ImGui 컨텍스트 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->Build();
    ImGui::StyleColorsDark(); // 다크 모드 설정

    // 2. Win32 초기화
    ImGui_ImplWin32_Init(hwnd);

    // 3. DX12용 Descriptor Heap 생성 (폰트 텍스처용)
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pSrvDescHeap));

    // 4. DX12 초기화
    // 인자: 디바이스, 버퍼 개수(보통 2 or 3), RTV 포맷, 
    //       SRV힙, 폰트텍스처 CPU 핸들, 폰트텍스처 GPU 핸들
    ImGui_ImplDX12_Init(device, numFramesInFlight,
        rtvFormat,
        m_pSrvDescHeap,
        m_pSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_pSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void CImGuiManager::Update()
{
    // 순서 중요!
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // =========================================================
    // [여기서부터 로그인 창 디자인 시작!]
    // =========================================================

    // 창 위치와 크기를 고정하고 싶다면 (선택 사항)
    ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);

    // 로그인 창 시작
    ImGui::Begin("Login", NULL, ImGuiWindowFlags_NoCollapse);

    static char id[64] = "";
    static char pw[64] = "";

    ImGui::Text("!! Welcome to UNDEAD WORLD !!"); // 라벨
    ImGui::Separator(); // 줄 긋기
    ImGui::Spacing();   // 여백

    // 입력 필드
    ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
    ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password); // *** 처리

    ImGui::Spacing();
    ImGui::Spacing();

    // 버튼 생성 및 클릭 이벤트 처리
    if (ImGui::Button("Connect & Login", ImVec2(280, 50)))
    {
        // [중요] 여기서 서버 개발자님의 실력을 발휘할 차례!
        // 버튼을 누르면 이 블록 안으로 들어옵니다.

        // 1. CString 변환 (필요하다면)
        // 2. 패킷 생성 (C_LOGIN 패킷 등)
        // 3. SendPacket 호출!
        // 예: NetworkManager::GetInstance().SendLoginPacket(id, pw);

        // 일단 확인용으로 콘솔 출력
        printf("Login Requested! ID: %s\n", id);
    }

    ImGui::End(); // 창 끝
    // =========================================================

    // === 여기서부터 UI 코드를 작성하면 됩니다 ===
    // 테스트용 데모 창 띄우기 (나중에 로그인 창 만들 때 지우세요)
    //ImGui::ShowDemoWindow();
}

void CImGuiManager::Render(ID3D12GraphicsCommandList* cmdList)
{
    // 렌더링 데이터 조립
    ImGui::Render();

    // DX12 커맨드 리스트에 그리기 명령 기록
    // 주의: 팀원 코드에서 이미 SetDescriptorHeaps를 호출했을 수도 있는데,
    // ImGui는 자신의 힙이 필요하므로 다시 설정해줍니다.
    ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvDescHeap };
    cmdList->SetDescriptorHeaps(1, ppHeaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void CImGuiManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (m_pSrvDescHeap) { m_pSrvDescHeap->Release(); m_pSrvDescHeap = nullptr; }
}