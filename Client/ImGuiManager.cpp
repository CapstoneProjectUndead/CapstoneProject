#include "stdafx.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "SessionManager.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "User.h"

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

    // ==================================
    // [추가] 한글 폰트 로드 (맑은 고딕 사용)
    // ==================================
    // 윈도우에 기본으로 있는 맑은 고딕(malgun.ttf)을 가져옵니다.
    // 18.0f는 폰트 크기입니다.
    // GetGlyphRangesKorean()이 핵심입니다! (한글 자모음+완성형 포함)
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgun.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    title_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\VINERITC.TTF", 270.0f);
    title_font2 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ELEPHNT.TTF", 80.0f);
    // ============================================================

    io.Fonts->Build();

    // 2. Win32 초기화
    ImGui_ImplWin32_Init(hwnd);

    // 3. DX12용 Descriptor Heap 생성 (폰트 텍스처용)
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv_desc_heap));

    // 4. DX12 초기화
    // 인자: 디바이스, 버퍼 개수(보통 2 or 3), RTV 포맷, 
    //       SRV힙, 폰트텍스처 CPU 핸들, 폰트텍스처 GPU 핸들
    ImGui_ImplDX12_Init(device, numFramesInFlight,
        rtvFormat,
        srv_desc_heap,
        srv_desc_heap->GetCPUDescriptorHandleForHeapStart(),
        srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

    // 임시
    RoomListInfo info{};
    info.total_player = 1;
    info.is_in_game = false;
    info.max_player = 4;
    info.room_cnt = 1;
    info.room_id = 1;
    COPY_STRING(info.room_name, "보물 파밍 가자");
    room_vec.push_back(info);

    info.total_player = 1;
    info.is_in_game = false;
    info.max_player = 4;
    info.room_cnt = 1;
    info.room_id = 2;
    COPY_STRING(info.room_name, "초보 환영");
    room_vec.push_back(info);
}

void CImGuiManager::Update()
{
    // 순서 중요!
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawTitleUI();
    //DrawRoomListUI();

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
    ID3D12DescriptorHeap* ppHeaps[] = { srv_desc_heap };
    cmdList->SetDescriptorHeaps(1, ppHeaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void CImGuiManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (srv_desc_heap) { 
        srv_desc_heap->Release(); 
        srv_desc_heap = nullptr; 
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
    // Title Scene 에서만 로그인 UI를 그린다. 아니라면 return!
    CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
    if (currentScene->GetSceneType() != SCENE_TYPE::TITLE)
        return;

    DrawTitle();

    if (!show_login_window && !show_sign_window && !is_login_loading && !is_signup_loading) {
        // =========================================================
        // 1. 메인 버튼 창 (고정시키기)
        // =========================================================

        // Always로 설정하면 매 프레임 위치/크기를 강제로 덮어씌웁니다.
        ImGui::SetNextWindowPos(ImVec2(300, 350), ImGuiCond_Always);

        ImGui::SetNextWindowBgAlpha(0.0f);

        // 플래그 조합 (OR 연산자 | 사용)
        // ImGuiWindowFlags_NoMove: 마우스로 드래그 불가
        // ImGuiWindowFlags_NoResize: 크기 조절 불가
        // ImGuiWindowFlags_NoCollapse: 최소화(접기) 불가
        // ImGuiWindowFlags_NoTitleBar: (옵션) 제목 표시줄을 없애서 그냥 버튼만 둥둥 떠있게 함
        ImGuiWindowFlags mainBtnFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;

        // [추가할 플래그]
        // ImGuiWindowFlags_NoBringToFrontOnFocus : "포커스를 받아도(클릭해도) 맨 앞으로 나오지 마라"
        mainBtnFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        // 배경 테두리 없애기
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("Main Menu", NULL, mainBtnFlags)) {

            // =========================================================
            // [추가] 비활성화 조건 설정
            // 로그인 창(show_login_window)이나 회원가입 창(show_sign_window) 중 
            // 하나라도 켜져 있으면 true가 되어 버튼들이 잠깁니다.
            // =========================================================
            bool should_disable = show_login_window || show_sign_window || is_login_loading || is_signup_loading;

            // 비활성화 시작! (여기서부터 그려지는 모든 UI는 잠김 상태가 됨)
            ImGui::BeginDisabled(should_disable);

            //===========
            // 로그인 버튼
            //===========

            // 1. 색상 변경 시작 (Push)
            // 인자: (바꿀 대상, 색상값(R, G, B, A)) - 0.0f ~ 1.0f 사이 값
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));        // 평소 색 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f)); // 마우스 올렸을 때 
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // 눌렀을 때 

            if (ImGui::Button((const char*)u8"로그인", ImVec2(200, 55))) {
                show_login_window = true;
                show_sign_window = false;
            }

            // 텍스처 불러오면 이 함수를 호출할 것.
            //if (ImageButtonWithText(0, (const char*)u8"로그인", ImVec2(200, 55))) {
            //    show_login_window = true;
            //    show_sign_window = false;
            //}

            // 3. 색상 복구 (Pop)
            // Push를 3번 했으니 Pop도 3번 해야 함!
            ImGui::PopStyleColor(3);

            // 버튼 사이에 간격 좀 주기
            //ImGui::SameLine(); // 옆으로 나란히 배치하고 싶으면 이 줄 추가 (없으면 아래로 배치)
            ImGui::Spacing();
            ImGui::Spacing();

            //============
            // 회원가입 버튼
            //============

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));        // 평소 색 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f)); // 마우스 올렸을 때 
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // 눌렀을 때 

            if (ImGui::Button((const char*)u8"회원가입", ImVec2(200, 55))) {
                show_sign_window = true;
                show_login_window = false;
            }
            ImGui::PopStyleColor(3);

            ImGui::Spacing();
            ImGui::Spacing();

            //============
            // 나가기 버튼
            //============

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));        // 평소 색 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f)); // 마우스 올렸을 때 
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // 눌렀을 때 

            if (ImGui::Button((const char*)u8"나가기", ImVec2(200, 55))) {
                g_run = false;
            }
            ImGui::PopStyleColor(3);

            // =========================================================
            // 비활성화 종료
            // 반드시 EndDisabled를 호출해야 이후 UI(로그인 창 등)는 정상적으로 클릭됩니다.
            // =========================================================
            ImGui::EndDisabled();

            ImGui::End();
        }

        // 이걸 하지 않으면 다른 UI들도 테두리가 없어진다.
        ImGui::PopStyleVar();
    }

    // =========================================================
    // 2. 로그인 창 (고정시키기)
    // =========================================================
    if (show_login_window) {

        // 화면 중앙에 고정 (해상도에 따라 좌표는 조절하세요)
        ImGui::SetNextWindowPos(ImVec2(210, 260), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(390, 220), ImGuiCond_Always); // 크기 고정

        // 로그인 창용 플래그 (제목 표시줄은 남겨둠)
        ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin((const char*)u8"로그인", &show_login_window, loginFlags)) {

            // 이미지 그리기
            //if (m_loginImageHandle.ptr != 0) {
            //    // 이미지 가운데 정렬을 위한 계산 (창 너비 - 이미지 너비) / 2
            //    float windowWidth = ImGui::GetWindowSize().x;
            //    float imageWidth = 380.0f;
            //    ImGui::SetCursorPosX((windowWidth - imageWidth) * 0.5f);
            //    ImGui::Image((ImTextureID)m_loginImageHandle.ptr, ImVec2(imageWidth, 100));
            //}

            //ImGui::Spacing();

            static char id[64] = "";
            static char pw[64] = "";

            ImGui::Text((const char*)u8"ID / PW 를 입력하세요.");
            ImGui::Separator();

            // 가로 길이를 200픽셀로 고정
            //ImGui::SetNextItemWidth(200.0f);
            
            // 만약 창 너비의 50%만큼만 쓰고 싶다면?
            //ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

            // 입력창 배경색을 회색으로 설정 (R, G, B를 0.2f로 설정하면 어두운 회색이 됩니다)
            // 인자: (대상, ImVec4(R, G, B, A))
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));          // 평상시 배경
            
            // 이후에 나오는 모든 아이템의 길이를 230으로 설정
            ImGui::PushItemWidth(230.0f);

            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
            ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);

            // 설정 해제 (원래 길이로 복구)
            ImGui::PopItemWidth();

            // 스타일 복구 (3개를 Push했으니 3개를 Pop)
            ImGui::PopStyleColor(1);

            ImGui::Spacing();
            ImGui::Spacing();

            // 버튼도 가운데 정렬
            float btnWidth = 380.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

            if (ImGui::Button("Connect & Login", ImVec2(btnWidth, 50)))
            {
                printf("Login Requested! ID: %s\n", id);
                // SendPacket(id, pw)...

                C_LOGIN loginPkt;
                COPY_STRING(loginPkt.id, id);
                COPY_STRING(loginPkt.password, pw);
                auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);

                auto session = CSessionManager::GetInstance().GetServerSession();
                if (session)
                    CSessionManager::GetInstance().GetServerSession()->DoSend(sendBuffer);

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
        ImGui::SetNextWindowPos(ImVec2(210, 260), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(390, 230), ImGuiCond_Always); // 크기 고정

        // 로그인 창용 플래그 (제목 표시줄은 남겨둠)
        ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin((const char*)u8"회원가입", &show_sign_window, loginFlags)) {
            // 이미지 그리기
            //if (m_loginImageHandle.ptr != 0) {
            //    // 이미지 가운데 정렬을 위한 계산 (창 너비 - 이미지 너비) / 2
            //    float windowWidth = ImGui::GetWindowSize().x;
            //    float imageWidth = 380.0f;
            //    ImGui::SetCursorPosX((windowWidth - imageWidth) * 0.5f);
            //    ImGui::Image((ImTextureID)m_loginImageHandle.ptr, ImVec2(imageWidth, 100));
            //}

            //ImGui::Spacing();

            static char id[64] = "";
            static char pw[64] = "";
            static char name[64] = "";

            ImGui::Text((const char*)u8"ID / PW / Name 을 입력하세요.");
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));          // 평상시 배경

            ImGui::PushItemWidth(230.0f);

            ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
            ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
            ImGui::InputText("Name", name, IM_ARRAYSIZE(name));

            ImGui::PopItemWidth();

            ImGui::PopStyleColor(1);

            ImGui::Spacing();
            ImGui::Spacing();

            // 버튼도 가운데 정렬
            float btnWidth = 380.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

            if (ImGui::Button((const char*)u8"가입 신청", ImVec2(btnWidth, 50))) {

                printf("가입 신청 ID: %s\n", id);
                // SendPacket(id, pw)...

                C_SIGNUP signUpPkt;
                COPY_STRING(signUpPkt.id, id);
                COPY_STRING(signUpPkt.password, pw);
                COPY_STRING(signUpPkt.name, name);
                auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_SIGNUP>(signUpPkt);

                auto session = CSessionManager::GetInstance().GetServerSession();
                if (session)
                    CSessionManager::GetInstance().GetServerSession()->DoSend(sendBuffer);

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
    if (is_login_loading || is_signup_loading) {

        ImGui::OpenPopup("LoadingPopup");

        // 화면 중앙 설정
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        // 팝업 그리기 (코드는 딱 한 번만 존재함!)
        if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {
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
            if (ImGui::Button("Cancel")) {

                // 무엇이 로딩 중이었든 둘 다 꺼버림
                is_login_loading = false;
                is_signup_loading = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

void CImGuiManager::DrawTitleUI()
{
    // Title Scene 에서만 로그인 UI를 그린다. 아니라면 return!
    CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
    if (currentScene->GetSceneType() != SCENE_TYPE::TITLE)
        return;

    // 배경과 타이틀 그리기
    DrawTitle();

    // =========================================
    // 1. 메인 버튼 창 (상태에 따라 버튼 내용이 바뀜)
    // =========================================
    if (!show_login_window && !show_sign_window && !is_login_loading && !is_signup_loading && !show_room_list_window) {
        DrawTitleMainWindow();
    }

    // ============
    // 2. 로그인 창
    // ============
    if (show_login_window) {
        DrawSignInWidow();
    }

    // =============
    // 3. 회원가입 창
    // =============
    if (show_sign_window) {
        DrawSignUpWindow();
    }

    // ============
    // 4. 로딩 팝업
    // ============

    if (is_login_loading || is_signup_loading) {
        DrawLoadingPopUp();
    }

    // =================
    // 5. 로딩 팝업 결과
    // =================

    if (is_signup_success || is_signin_success) {
        DrawLoadingPopUpResult();
    }

    // ==============
    // 6. 룸 매칭 화면
    // ==============
    if (show_room_list_window) {
        DrawRoomListUI();
    }
}

void CImGuiManager::DrawTitleMainWindow()
{
    // 멀티플레이 메뉴(로그인/가입)를 보여줄지 여부
    // false: [싱글] [멀티] [나가기]
    // true : [로그인] [회원가입] [뒤로가기]
    static bool show_multiplayer_select = false;

    // Always로 설정하면 매 프레임 위치/크기를 강제로 덮어씌웁니다.
    ImGui::SetNextWindowPos(ImVec2(300, 350), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f); // 배경 투명

    // 플래그 설정
    ImGuiWindowFlags mainBtnFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
    mainBtnFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

    // 배경 테두리 없애기
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Main Menu", NULL, mainBtnFlags))
    {
        // [비활성화 조건] 
        // 로그인/회원가입 창이 떴거나 로딩 중이면 뒤에 있는 버튼 클릭 금지
        bool should_disable = show_login_window || show_sign_window || is_login_loading || is_signup_loading;
        ImGui::BeginDisabled(should_disable);

        // 버튼 스타일 공통 적용 (회색)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));

        // =====================================
        // [CASE 1] 초기 화면: 싱글 / 멀티 / 나가기
        // =====================================
        if (!show_multiplayer_select) {
            DrawFirstMenuButton(show_multiplayer_select);
        }
        // ====================================================
        // [CASE 2] 멀티 메뉴(오프라인): 로그인 / 회원가입 / 뒤로가기
        // ====================================================
        else if (!is_online) {
            DrawSecondMenuButton(show_multiplayer_select);
        }
        // ===================================================
        // [CASE 3] 멀티 메뉴(온라인): 방 검색 / 로그아웃 / 뒤로가기
        // ===================================================
        else {
            DrawThirdMenuButton(show_multiplayer_select);
        }

        ImGui::PopStyleColor(3); // 스타일 복구
        ImGui::EndDisabled(); // 비활성화 종료
        ImGui::End();
    }
    ImGui::PopStyleVar(); // 테두리 스타일 복구
}

void CImGuiManager::DrawFirstMenuButton(bool& menu)
{
    // 1. 싱글 플레이
    if (ImGui::Button((const char*)u8"싱글 플레이", ImVec2(200, 55))) {
        printf("싱글 플레이 로그인 시도.\n");
        // 바로 LobbyScene으로 입장
        CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
    }
    ImGui::Spacing(); ImGui::Spacing();

    // 2. 멀티 플레이 (누르면 메뉴 변경)
    if (ImGui::Button((const char*)u8"멀티 플레이", ImVec2(200, 55))) {
        menu = true; // 상태 변경!
    }
    ImGui::Spacing(); ImGui::Spacing();

    // 3. 나가기
    if (ImGui::Button((const char*)u8"나가기", ImVec2(200, 55))) {
        PostQuitMessage(0); // 프로그램 종료
    }
}

void CImGuiManager::DrawSecondMenuButton(bool& menu)
{
    // 1. 로그인
    if (ImGui::Button((const char*)u8"로그인", ImVec2(200, 55))) {
        show_login_window = true;
        show_sign_window = false;
    }
    ImGui::Spacing(); ImGui::Spacing();

    // 2. 회원가입
    if (ImGui::Button((const char*)u8"회원가입", ImVec2(200, 55))) {
        show_sign_window = true;
        show_login_window = false;
    }
    ImGui::Spacing(); ImGui::Spacing();

    // 3. 뒤로가기 (누르면 초기 화면으로)
    if (ImGui::Button((const char*)u8"뒤로가기", ImVec2(200, 55))) {
        menu = false; // 상태 원복!
    }
}

void CImGuiManager::DrawThirdMenuButton(bool& menu)
{
    // 1. 방 검색
    if (ImGui::Button((const char*)u8"방 검색", ImVec2(200, 55))) {
        show_login_window = false;
        show_sign_window = false;
        show_room_list_window = true;
    }
    ImGui::Spacing();  ImGui::Spacing();

    // 2. 로그아웃
    if (ImGui::Button((const char*)u8"로그아웃", ImVec2(200, 55))) {
        C_LOGOUT logOutPkt;
        auto serverSession = CSessionManager::GetInstance().GetServerSession();
        auto user = serverSession->GetUser();
        logOutPkt.user_id = user->GetUserID();
        auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGOUT>(logOutPkt);
        serverSession->DoSend(sendBuffer);
    }
    ImGui::Spacing();  ImGui::Spacing();

    // 3. 뒤로가기 (누르면 초기 화면으로)
    if (ImGui::Button((const char*)u8"뒤로가기", ImVec2(200, 55))) {
        menu = false; // 상태 원복!
    }
}

void CImGuiManager::DrawTitle()
{
    if (is_title_draw && !show_room_list_window) {

        // =========================================================
        // [배경 색 덧칠하기]
        // =========================================================
        // 1. 전체 화면의 크기를 가져옵니다.
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;

        // 2. 배경 전용 도화지(DrawList)를 가져와서 사각형을 그립니다.
        // 인자: (좌상단 좌표, 우하단 좌표, 색상(RGBA))
        // 색상 예: ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 1.0f)) -> 어두운 회색
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(0, 0),
            screenSize,
            ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 1.0f))
        );

        // =========================================================
        //  게임 타이틀 "UNDEAD" 그리기
        // =========================================================

        // [고정] 위치와 크기 강제 고정 (Always)
        // 화면 중앙쯤에 예쁘게 배치 (좌표는 크기에 맞게 수정)
        ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(760, 210), ImGuiCond_Always);

        // [배경 제거] 배경색을 완전 투명하게 (Alpha = 0.0f)
        ImGui::SetNextWindowBgAlpha(0.0f);

        // 3. [플래그 설정] 
        // ImGuiWindowFlags_NoDecoration: 타이틀바(Debug 글씨), 테두리, 리사이즈 그립 등을 싹 없애는 강력한 플래그
        // ImGuiWindowFlags_NoMove: 마우스로 이동 불가
        // ImGuiWindowFlags_NoBringToFrontOnFocus: 클릭해도 앞으로 튀어나오지 않음
        ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // 창 시작 ("Main Menu"라는 이름은 내부 식별용일 뿐, NoDecoration 때문에 화면엔 안 나옴)
        if (ImGui::Begin("Title", NULL, mainFlags)) {

            // 타이틀 폰트로 갈아 끼우기 (없으면 기본 폰트 사용)
            if (title_font)
                ImGui::PushFont(title_font);

            const char* titleText = "UNDEAD";

            // 글자 너비 계산 (가운데 정렬을 위해)
            // 폰트를 Push한 상태에서 계산해야 정확한 크기가 나옵니다.
            float textWidth = ImGui::CalcTextSize(titleText).x;
            float windowWidth = ImGui::GetWindowSize().x;

            // 커서를 가운데로 이동: (창 너비 - 글자 너비) / 2
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

            // 빨간색으로 "UNDEAD" 출력! (좀비 게임이니까 빨강 추천)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            ImGui::Text(titleText);
            ImGui::PopStyleColor(); // 색상 복구

            // 폰트 복구 (이제부터 그리는 건 다시 기본 폰트로)
            if (title_font)
                ImGui::PopFont();

            // =========================================================

            ImGui::Spacing();
            ImGui::Spacing();
            //ImGui::Separator(); // 타이틀과 버튼 사이에 줄 긋기 (선택사항)
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
}

void CImGuiManager::DrawSignInWidow()
{
    ImGui::SetNextWindowPos(ImVec2(210, 260), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(390, 220), ImGuiCond_Always);
    ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin((const char*)u8"로그인", &show_login_window, loginFlags)) {
        static char id[64] = "";
        static char pw[64] = "";

        ImGui::Text((const char*)u8"ID / PW 를 입력하세요.");
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::PushItemWidth(230.0f);
        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(1);

        ImGui::Spacing(); ImGui::Spacing();

        float btnWidth = 380.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        if (ImGui::Button("Connect & Login", ImVec2(btnWidth, 50))) {

            printf("Login Requested! ID: %s\n", id);

            C_LOGIN loginPkt;
            COPY_STRING(loginPkt.id, id);
            COPY_STRING(loginPkt.password, pw);
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);

            auto session = CSessionManager::GetInstance().GetServerSession();
            if (session)
                CSessionManager::GetInstance().GetServerSession()->DoSend(sendBuffer);

            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));
            show_login_window = false;
            is_login_loading = true;
        }
    }
    ImGui::End();
}

void CImGuiManager::DrawSignUpWindow()
{
    ImGui::SetNextWindowPos(ImVec2(210 * G_RATIO_X, 260 * G_RATIO_Y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(390, 230), ImGuiCond_Always);
    ImGuiWindowFlags loginFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin((const char*)u8"회원가입", &show_sign_window, loginFlags)) {
        static char id[64] = "";
        static char pw[64] = "";
        static char name[64] = "";

        ImGui::Text((const char*)u8"ID / PW / Name 을 입력하세요.");
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));          // 평상시 배경

        ImGui::PushItemWidth(230.0f);

        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::InputText("Name", name, IM_ARRAYSIZE(name));

        ImGui::PopItemWidth();

        ImGui::PopStyleColor(1);

        ImGui::Spacing();
        ImGui::Spacing();

        // 버튼도 가운데 정렬
        float btnWidth = 380.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        if (ImGui::Button((const char*)u8"가입 신청", ImVec2(btnWidth, 50))) {

            printf("가입 신청 ID: %s\n", id);

            C_SIGNUP signUpPkt;
            COPY_STRING(signUpPkt.id, id);
            COPY_STRING(signUpPkt.password, pw);
            COPY_STRING(signUpPkt.name, name);
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_SIGNUP>(signUpPkt);

            auto session = CSessionManager::GetInstance().GetServerSession();
            if (session)
                CSessionManager::GetInstance().GetServerSession()->DoSend(sendBuffer);

            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));
            memset(name, 0, sizeof(name));
            show_sign_window = false;
            is_signup_loading = true;
        }
    }
    ImGui::End();
}

void CImGuiManager::DrawLoadingPopUp()
{
    if (!ImGui::IsPopupOpen("LoadingPopup")) {
        ImGui::OpenPopup("LoadingPopup");
    }

    // BeginPopupModal은 OpenPopup("LoadingPopup")이 호출된 적이 있어야만 true를 반환합니다.
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        // 뱅글이 그리기
        LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);

        ImGui::Spacing(); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

        if (is_login_loading)
            ImGui::Text((const char*)u8"로그인 중입니다...");
        else
            ImGui::Text((const char*)u8"가입 처리 중입니다...");

        ImGui::Spacing();

        if (ImGui::Button("Cancel")) {
            is_login_loading = false;
            is_signup_loading = false;
            ImGui::CloseCurrentPopup();

            // 이건 테스트 임시용 (무조건 지울 것!)
            //show_room_list_window = true;
        }

        ImGui::EndPopup();
    }
}

void CImGuiManager::DrawLoadingPopUpResult()
{
    if (!ImGui::IsPopupOpen("SuccessPopup")) {
        ImGui::OpenPopup("SuccessPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("SuccessPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        if (is_signup_success && !is_signin_success)
            ImGui::Text((const char*)u8"가입 성공!");
        else if (!is_signup_success && !is_signin_success)
            ImGui::Text((const char*)u8"가입 실패...!");

        if (is_signin_success && !is_signup_success)
            ImGui::Text((const char*)u8"로그인 성공!");
        else if (!is_signin_success && !is_signup_success)
            ImGui::Text((const char*)u8"로그인 실패...!");

        ImGui::Spacing();

        if (ImGui::Button((const char*)u8"확인")) {

            // 가입을 시도했고, 가입 성공하면
            if (is_signup_success) {
                is_signup_success = false;
            }

            // 로그인을 시도했고, 로그인 성공하면
            if (is_signin_success) {
                is_signin_success = false;

                // 룸 매칭 UI 화면으로 전환
                show_room_list_window = true;

                // 온라인 상태로 바꿈
                is_online = true;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CImGuiManager::DrawRoomListUI()
{
    CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
    if (currentScene->GetSceneType() != SCENE_TYPE::TITLE) // 혹은 LOBBY
        return;

    // 1. 전체 배경색 (파란색)
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0, 0), ImGui::GetIO().DisplaySize,
        ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.4f)));

    // =================
    //  메인 윈도우 시작
    // =================
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f); // 투명 배경
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("MatchingWindow", NULL, windowFlags))
    {
        // -----------------------------------------------------
        // 1. 타이틀 "UNDEAD"
        // -----------------------------------------------------
        ImGui::Spacing(); 
        ImGui::Spacing(); 
        ImGui::Spacing();

        if (title_font2) 
            ImGui::PushFont(title_font2);

        const char* titleText = "UNDEAD";
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(titleText).x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

        // 글자색 검정 (0,0,0,1)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text(titleText);
        ImGui::PopStyleColor();

        if (title_font2) 
            ImGui::PopFont();

        ImGui::Spacing(); 
        ImGui::Spacing();

        // -----------------
        // 2. 방 목록 테이블
        // -----------------
        float tableWidth = 700.0f * G_RATIO_X;
        float tableHeight = 300.0f * G_RATIO_Y;

        ImGui::SetCursorPosX((windowWidth - tableWidth) * 0.5f);

        // 테이블 배경 (Child)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f, 0.9f, 0.9f, 0.7f));

        if (ImGui::BeginChild("TableArea", ImVec2(tableWidth, tableHeight), true, ImGuiWindowFlags_None)) {

            static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

            if (ImGui::BeginTable("RoomTable", 3, tableFlags)) {

                ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn((const char*)u8"방 제목", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn((const char*)u8"인원", ImGuiTableColumnFlags_WidthFixed, 100.0f);

                // 헤더 글자색 검정으로 변경
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 검정
                ImGui::TableHeadersRow();
                ImGui::PopStyleColor(2);

                // 내용물 스타일
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.9f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 0.0f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 검정

                for (const auto& room : room_vec) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%u", room.room_id);

                    std::string utf8Name = CP949ToUTF8(room.room_name);

                    // "방제목##방ID" 형태로 문자열을 만듭니다.
                    // 예: "초보환영##10" -> 화면엔 "초보환영"만 보이고, ID는 10번으로 구분됨
                    std::string uniqueLabel = utf8Name + "##" + std::to_string(room.room_id);

                    ImGui::TableSetColumnIndex(1);

                    bool is_selected = (selected_room_id == room.room_id);
                    if (ImGui::Selectable(uniqueLabel.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_room_id = room.room_id;
                        printf("room ID: %d\n", selected_room_id);
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        selected_room_id = room.room_id;
                        // SendEnterRoomPacket(selected_room_id); 
                    }

                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", room.total_player, room.max_player);
                }
                ImGui::PopStyleColor(4);
                ImGui::EndTable();
            }

            // ===============================
            // 테이블 영역 빈 곳 클릭 시 선택 해제
            // ===============================
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                selected_room_id = 0;
            }

        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // ChildBg 복구

        // ---------------
        // 3. 새로고침 버튼
        // ---------------
        float refreshBtnSize = 60.0f;
        ImGui::SetCursorPosX((windowWidth + tableWidth) * 0.5f - refreshBtnSize);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
        // 버튼 글자색 지정을 위해 스타일 추가 (검정)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (ImGui::Button((const char*)u8"새로\n고침", ImVec2(refreshBtnSize, refreshBtnSize))) {
            // 새로고침 로직
            printf("새로고침\n");
        }
        ImGui::PopStyleColor(2); // Button, Text


        // ----------------
        // 4. 하단 버튼 3개
        // ----------------
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

        float btnWidth = 200.0f * G_RATIO_X;
        float btnHeight = 60.0f * G_RATIO_Y;
        float spacing = 50.0f * G_RATIO_X;

        float totalBtnWidth = (btnWidth * 3) + (spacing * 2);
        ImGui::SetCursorPosX((windowWidth - totalBtnWidth) * 0.5f);

        // 버튼 스타일
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95f, 0.55f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.65f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.45f, 0.15f, 1.0f));
        // 버튼 글자색 검정
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (ImGui::Button((const char*)u8"방 만들기", ImVec2(btnWidth, btnHeight))) {
            ImGui::OpenPopup("CreateRoom");
        }
        ImGui::SameLine(0, spacing);

        if (ImGui::Button((const char*)u8"방 입장", ImVec2(btnWidth, btnHeight))) {
            if (selected_room_id != 0) {
                // 입장 로직
                ImGui::OpenPopup("EnterLoading");
            }
        }
        ImGui::SameLine(0, spacing);

        if (ImGui::Button((const char*)u8"뒤로 가기", ImVec2(btnWidth, btnHeight))) {
            // 뒤로가기 로직
            show_room_list_window = false;
        }

        ImGui::PopStyleColor(4);

        // ----------------
        // 5. 팝업 (방 생성)
        // ----------------
        ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        // 팝업 내부 글자색도 검정으로 설정하기 위해 스타일 Push
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (ImGui::BeginPopupModal("CreateRoom", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char roomName[128] = "";

            ImGui::Text((const char*)u8"생성할 방 제목을 입력하세요.");
            ImGui::Spacing();

            ImGui::PushItemWidth(300.0f);
            ImGui::InputText("##RoomName", roomName, IM_ARRAYSIZE(roomName));
            ImGui::PopItemWidth();

            ImGui::Spacing(); 
            ImGui::Separator(); 
            ImGui::Spacing();

            if (ImGui::Button((const char*)u8"생성", ImVec2(120, 40))) {

                static int roomCounter = 1;

                // 방 이름이 비어있는지 체크
                if (strlen(roomName) == 0) {
                    // 없다면 아래처럼 기본 이름을 세팅합니다.
                    sprintf_s(roomName, sizeof(roomName), "Unknown Room %d", roomCounter++);
                }

                printf("방 생성 요청: %s\n", roomName);

                C_CreateRoom createPkt;
                auto serverSession = CSessionManager::GetInstance().GetServerSession();
                auto user = serverSession->GetUser();
                createPkt.user_id = user->GetUserID();
                COPY_STRING(createPkt.room_name, roomName);
                auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_CreateRoom>(createPkt);
                serverSession->DoSend(sendBuffer);

                memset(roomName, 0, sizeof(roomName));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button((const char*)u8"취소", ImVec2(120, 40))) {
                memset(roomName, 0, sizeof(roomName));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(); // 팝업 텍스트 색상 복구

        //================
        // 방 입장 팝업 생성
        //================
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (ImGui::BeginPopupModal("EnterLoading", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

            LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);

            if (ImGui::Button((const char*)u8"취소", ImVec2(120, 40))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(); // 팝업 텍스트 색상 복구

        // =====================================
        //  파란 배경(메인 윈도우) 클릭 시 선택 해제
        // =====================================
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            selected_room_id = 0;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

bool ImageButtonWithText(long long texturePtr, const char* label, const ImVec2& size)
{
    // 1. 현재 커서 위치(버튼이 그려질 위치)를 저장해둡니다.
    ImVec2 p = ImGui::GetCursorPos();

    // 2. [1층] 이미지를 그립니다.
    // 텍스처가 있으면 그리고, 없으면(0이면) 그냥 빈 공간만 차지하게 둡니다.
    if (texturePtr != 0) {
        ImGui::Image((ImTextureID)texturePtr, size);
    }
    else {
        // 이미지가 아직 로드 안 됐을 때를 대비해 투명 박스 처리
        ImGui::Dummy(size);
    }

    // 3. [2층] 커서를 다시 아까 저장한 위치(이미지 시작점)로 되돌립니다.
    // 이걸 안 하면 버튼이 이미지 아래에 그려집니다.
    ImGui::SetCursorPos(p);

    // 4. [3층] 투명 버튼을 그립니다.
    // 배경색을 투명(Alpha=0)하게 해서 뒤에 있는 이미지가 보이게 합니다.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    // 마우스 올렸을 때 살짝 하얗게 빛나게 (Highlight 효과)
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.2f));

    // 눌렀을 때 좀 더 진하게
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.4f));

    // 실제 버튼 생성 (이름과 크기는 그대로 전달)
    bool clicked = ImGui::Button(label, size);

    // 스타일 복구
    ImGui::PopStyleColor(3);

    return clicked; // 클릭 여부 반환
}

std::string CP949ToUTF8(const std::string& strCP949)
{
    if (strCP949.empty()) return "";

    // 1. CP949 -> WideChar (UTF-16)
    int nwLen = MultiByteToWideChar(949, 0, strCP949.c_str(), -1, NULL, 0);
    wchar_t* pwBuf = new wchar_t[nwLen];
    MultiByteToWideChar(949, 0, strCP949.c_str(), -1, pwBuf, nwLen);

    // 2. WideChar -> UTF-8
    int nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, 0, NULL, NULL);
    char* pBuf = new char[nLen];
    WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, pBuf, nLen, NULL, NULL);

    std::string strUTF8(pBuf);

    delete[] pwBuf;
    delete[] pBuf;

    return strUTF8;
}
