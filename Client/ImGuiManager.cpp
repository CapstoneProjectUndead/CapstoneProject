#include "stdafx.h"
#include "ImGuiManager.h"

#undef min
#undef max

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
    ImGui::StyleColorsLight(); // 다크 모드 설정

    // ============================================================
    // [추가] 한글 폰트 로드 (맑은 고딕 사용)
    // ============================================================
    // 윈도우에 기본으로 있는 맑은 고딕(malgun.ttf)을 가져옵니다.
    // 18.0f는 폰트 크기입니다.
    // GetGlyphRangesKorean()이 핵심입니다! (한글 자모음+완성형 포함)
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgun.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    // ============================================================

    io.Fonts->Build();

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

    DrawLogInUI();

    // === 여기서부터 UI 코드를 작성하면 됩니다 ===
    // 테스트용 데모 창 띄우기
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

    if (m_pSrvDescHeap) { 
        m_pSrvDescHeap->Release(); 
        m_pSrvDescHeap = nullptr; 
    }
}

void CImGuiManager::LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count, const float speed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 10.0f;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) {
        return;
    }

    // 시간 기반으로 회전 각도 계산
    const float t = g.Time;
    const auto degree_offset = 2.0f * IM_PI / circle_count;

    for (int i = 0; i < circle_count; ++i) {
        const auto x = indicator_radius * std::sin(degree_offset * i);
        const auto y = indicator_radius * std::cos(degree_offset * i);
        const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));

        ImVec4 color;
        color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
        color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
        color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
        color.w = 1.0f;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y), circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
    }
}

void CImGuiManager::DrawLogInUI()
{
    static bool show_login_window = false;
    static bool show_sign_window = false;
    static bool is_login_loading = false;
    static bool is_signup_loading = false;

    // =========================================================
    // 1. 메인 버튼 창 (고정시키기)
    // =========================================================

    // [핵심 1] Always로 설정하면 매 프레임 위치/크기를 강제로 덮어씌웁니다.
    ImGui::SetNextWindowPos(ImVec2(310, 320), ImGuiCond_Always);

    //ImGui::SetNextWindowBgAlpha(0.0f);

    // [핵심 2] 플래그 조합 (OR 연산자 | 사용)
    // ImGuiWindowFlags_NoMove: 마우스로 드래그 불가
    // ImGuiWindowFlags_NoResize: 크기 조절 불가
    // ImGuiWindowFlags_NoCollapse: 최소화(접기) 불가
    // ImGuiWindowFlags_NoTitleBar: (옵션) 제목 표시줄을 없애서 그냥 버튼만 둥둥 떠있게 함
    ImGuiWindowFlags mainBtnFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Main Menu", NULL, mainBtnFlags)) {
        if (ImGui::Button((const char*)u8"로그인", ImVec2(200, 40))) {
            show_login_window = true;
            show_sign_window = false;
        }

        // 버튼 사이에 간격 좀 주기
        //ImGui::SameLine(); // 옆으로 나란히 배치하고 싶으면 이 줄 추가 (없으면 아래로 배치)
        //ImGui::Spacing();

        if (ImGui::Button((const char*)u8"회원가입", ImVec2(200, 40))) {
            show_sign_window = true;
            show_login_window = false;
        }
        ImGui::End();
    }

    // =========================================================
    // 2. 로그인 창 (고정시키기)
    // =========================================================
    if (show_login_window) {

        // 화면 중앙에 고정 (해상도에 따라 좌표는 조절하세요)
        ImGui::SetNextWindowPos(ImVec2(210, 220), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 220), ImGuiCond_Always); // 크기 고정

        // 로그인 창용 플래그 (제목 표시줄은 남겨둠)
        ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin((const char*)u8"로그인", &show_login_window, loginFlags))
        {
            // 이미지 그리기
            //if (m_loginImageHandle.ptr != 0) {
            //    // 이미지 가운데 정렬을 위한 계산 (창 너비 - 이미지 너비) / 2
            //    float windowWidth = ImGui::GetWindowSize().x;
            //    float imageWidth = 380.0f;
            //    ImGui::SetCursorPosX((windowWidth - imageWidth) * 0.5f);
            //    ImGui::Image((ImTextureID)m_loginImageHandle.ptr, ImVec2(imageWidth, 100));
            //}

            ImGui::Spacing();

            static char id[64] = "";
            static char pw[64] = "";

            ImGui::Text((const char*)u8"ID / PW 를 입력하세요.");
            ImGui::Separator();

            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
            ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);

            ImGui::Spacing();
            ImGui::Spacing();

            // 버튼도 가운데 정렬
            float btnWidth = 380.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

            if (ImGui::Button("Connect & Login", ImVec2(btnWidth, 50)))
            {
                printf("Login Requested! ID: %s\n", id);
                // SendPacket(id, pw)...

                memset(id, 0, sizeof(id));
                memset(pw, 0, sizeof(pw));
                show_login_window = false;
                is_login_loading = true;
            }
        }

        ImGui::End();
    }
    else if (show_sign_window) {
        // 화면 중앙에 고정 (해상도에 따라 좌표는 조절하세요)
        ImGui::SetNextWindowPos(ImVec2(210, 220), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 230), ImGuiCond_Always); // 크기 고정

        // 로그인 창용 플래그 (제목 표시줄은 남겨둠)
        ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin((const char*)u8"회원가입", &show_sign_window, loginFlags))
        {
            // 이미지 그리기
            //if (m_loginImageHandle.ptr != 0) {
            //    // 이미지 가운데 정렬을 위한 계산 (창 너비 - 이미지 너비) / 2
            //    float windowWidth = ImGui::GetWindowSize().x;
            //    float imageWidth = 380.0f;
            //    ImGui::SetCursorPosX((windowWidth - imageWidth) * 0.5f);
            //    ImGui::Image((ImTextureID)m_loginImageHandle.ptr, ImVec2(imageWidth, 100));
            //}

            ImGui::Spacing();

            static char id[64] = "";
            static char pw[64] = "";
            static char name[64] = "";

            ImGui::Text((const char*)u8"ID / PW / Name 을 입력하세요.");
            ImGui::Separator();

            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
            ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
            ImGui::InputText("Name", name, IM_ARRAYSIZE(name));

            ImGui::Spacing();
            ImGui::Spacing();

            // 버튼도 가운데 정렬
            float btnWidth = 380.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

            if (ImGui::Button((const char*)u8"가입 신청", ImVec2(btnWidth, 50)))
            {
                printf("가입 신청 ID: %s\n", id);
                // SendPacket(id, pw)...

                memset(id, 0, sizeof(id));
                memset(pw, 0, sizeof(pw));
                memset(name, 0, sizeof(name));
                show_sign_window = false;
                is_signup_loading = true;
            }
        }
        ImGui::End();
    }

    // 둘 중 하나라도 true면 팝업을 열라고 명령
    if (is_login_loading || is_signup_loading)
    {
        ImGui::OpenPopup("LoadingPopup");
    }

    // 화면 중앙 설정
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    // 팝업 그리기 (코드는 딱 한 번만 존재함!)
    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        // 뱅글이 그리기
        LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);

        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // 상황에 따라 텍스트를 다르게 보여줄 수도 있음 (선택 사항)
        if (is_login_loading)
            ImGui::Text((const char*)u8"로그인 중입니다...");
        else
            ImGui::Text((const char*)u8"가입 처리 중입니다...");

        ImGui::Spacing();

        // 취소 버튼
        if (ImGui::Button("Cancel"))
        {
            // 무엇이 로딩 중이었든 둘 다 꺼버림
            is_login_loading = false;
            is_signup_loading = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}