#include "stdafx.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ServerSessionManager.h"
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
    ImGui::StyleColorsLight();

    // 폰트 로드
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgun.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    title_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\VINERITC.TTF", 270.0f);
    title_font2 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ELEPHNT.TTF", 80.0f);
    io.Fonts->Build();

    // Win32 & DX12 초기화
    ImGui_ImplWin32_Init(hwnd);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv_desc_heap));

    ImGui_ImplDX12_Init(device, numFramesInFlight,
        rtvFormat,
        srv_desc_heap,
        srv_desc_heap->GetCPUDescriptorHandleForHeapStart(),
        srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

    // 임시
    RoomInfo info{};
    info.total_player = 1;
    info.is_in_game = false;
    info.max_player = 4;
    info.room_id = 1;
    COPY_STRING(info.room_name, "보물 파밍 가자");
    room_vec.push_back(info);

    info.total_player = 1;
    info.is_in_game = false;
    info.max_player = 4;
    info.room_id = 2;
    COPY_STRING(info.room_name, "초보 환영");
    room_vec.push_back(info);
}

void CImGuiManager::Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // =========================================================
    // [IME 및 포커스 관리 로직]
    // =========================================================
    static bool lastInputState = false;
    bool currentInputState = IsUIInputEnabled();
    HWND hwnd = ghWnd;

    // 상태 변경 시에만 IME 제어
    if (currentInputState != lastInputState || need_reset_focus)
    {
        if (currentInputState) {
            EnableIME(hwnd);
        }
        else {
            DisableIME(hwnd);
        }

        if (need_reset_focus) {
            ImGuiContext& g = *GImGui;
            g.ActiveId = 0;
            ImGui::SetWindowFocus(nullptr);
            ImGui::GetIO().InputQueueCharacters.resize(0);
            ImGui::GetIO().ClearInputKeys();
            ResetIMEState(hwnd);

            need_reset_focus = false;
        }

        lastInputState = currentInputState;
    }

    // UI 그리기 시작
    DrawTitleUI();
}

void CImGuiManager::Render(ID3D12GraphicsCommandList* cmdList)
{
    ImGui::Render();
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

// =====================
// [IME 제어 헬퍼 함수들]
// =====================
bool CImGuiManager::IsUIInputEnabled()
{
    bool state = true;

    CScene* scene = CSceneManager::GetInstance().GetActiveScene();
    if (!scene)
        return false;

    // 타이틀 씬이면 무조건 입력 허용
    if (scene->GetSceneType() == SCENE_TYPE::TITLE)
        state = true;
    else
        state = false;

    // 로딩 중에는 입력 차단 (선택 사항)
    if ((ui_state == TitleUIState::Login) || (ui_state == TitleUIState::SignUp))
        state = false;

    return state;
}

void CImGuiManager::ResetIMEState(HWND hwnd)
{
    HIMC himc = ImmGetContext(hwnd);
    if (himc) {
        ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmSetOpenStatus(himc, FALSE);
        ImmReleaseContext(hwnd, himc);
    }
}

void CImGuiManager::DisableIME(HWND hwnd)
{
    if (m_hDefaultIMC != nullptr) 
        return;

    m_hDefaultIMC = ImmAssociateContext(hwnd, NULL);
}

void CImGuiManager::EnableIME(HWND hwnd)
{
    if (m_hDefaultIMC == nullptr) 
        return;

    ImmAssociateContext(hwnd, m_hDefaultIMC);
    m_hDefaultIMC = nullptr;
}

void CImGuiManager::ClearFocus(HWND hwnd)
{
    need_reset_focus = true;
    DisableIME(hwnd); // 즉시 차단
}

// ===================
// [UI 그리기 메인 로직]
// ===================

void CImGuiManager::DrawTitleUI()
{
    CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
    if (currentScene->GetSceneType() != SCENE_TYPE::TITLE) 
        return;

    // 1. 타이틀 로고 배경
    DrawTitle();

    // 2. 로딩 팝업 (최우선 순위)
    if (loading_type != LoadingType::None) {
        DrawLoadingPopUp();
    }

    // 3. 결과 팝업
    if (pop_up_result.is_visible) {
        DrawLoadingPopUpResult();
    }

    // 4. 상태에 따른 UI 분기
    switch (ui_state)
    {
    case TitleUIState::Main:
    case TitleUIState::MultiSelect:
        // 메인 메뉴
        DrawTitleMainWindow();
        break;

    case TitleUIState::Login:
        // 로그인 창
        DrawSignInWindow();
        break;

    case TitleUIState::SignUp:
        // 회원가입 창
        DrawSignUpWindow();
        break;

    case TitleUIState::RoomList:
        // 룸 매칭
        DrawRoomListUI();
        break;

    case TitleUIState::None:
        break;
    }

    // 방 생성 팝업은 RoomList 위에 뜸
    if (ui_state == TitleUIState::RoomList && show_room_create_popup) {
        DrawRoomCreatePopUp();
    }
}

void CImGuiManager::DrawTitle()
{
    if (!is_title_draw)
        return;

    // 배경 및 UNDEAD 텍스트 그리기 (기존 코드 유지)
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), screenSize, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 1.0f)));

    ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(760, 210), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Title", NULL, mainFlags)) {
        if (title_font) ImGui::PushFont(title_font);

        const char* titleText = "UNDEAD";
        float textWidth = ImGui::CalcTextSize(titleText).x;
        float windowWidth = ImGui::GetWindowSize().x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
        ImGui::Text(titleText);
        ImGui::PopStyleColor();

        if (title_font) ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// 메인 메뉴 & 멀티 선택 메뉴 통합 관리
void CImGuiManager::DrawTitleMainWindow()
{
    ImGui::SetNextWindowPos(ImVec2(300, 350), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags mainBtnFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Main Menu", NULL, mainBtnFlags))
    {
        // 로딩 중이거나 팝업 떠있으면 버튼 비활성화
        bool should_disable = (loading_type != LoadingType::None) || pop_up_result.is_visible;
        ImGui::BeginDisabled(should_disable);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));

        // ======================================
        // [State 1] 초기 화면: 싱글 / 멀티 / 나가기
        // ======================================
        if (ui_state == TitleUIState::Main)
        {
            if (ImGui::Button((const char*)u8"싱글 플레이", ImVec2(200, 55))) {
                StartLoading(LoadingType::SinglePlay);
                CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
            }
            ImGui::Spacing(); 
            ImGui::Spacing();

            if (ImGui::Button((const char*)u8"멀티 플레이", ImVec2(200, 55))) {
                SetUIState(TitleUIState::MultiSelect); // 상태 변경!
            }
            ImGui::Spacing(); 
            ImGui::Spacing();

            if (ImGui::Button((const char*)u8"게임 종료", ImVec2(200, 55))) {
                g_run = false;
            }
        }
        // =======================
        // [State 2] 멀티 선택 화면
        // =======================
        else if (ui_state == TitleUIState::MultiSelect)
        {
            if (!is_online) {
                // 오프라인 상태: 로그인 / 회원가입 / 뒤로가기
                if (ImGui::Button((const char*)u8"로그인", ImVec2(200, 55))) {
                    SetUIState(TitleUIState::Login);
                }
                ImGui::Spacing(); 
                ImGui::Spacing();

                if (ImGui::Button((const char*)u8"회원가입", ImVec2(200, 55))) {
                    SetUIState(TitleUIState::SignUp);
                }
            }
            else {
                // 온라인 상태: 방 검색 / 로그아웃
                if (ImGui::Button((const char*)u8"방 검색", ImVec2(200, 55))) {
                    is_title_draw = false;
                    SetUIState(TitleUIState::RoomList);
                }
                ImGui::Spacing(); 
                ImGui::Spacing();

                if (ImGui::Button((const char*)u8"로그아웃", ImVec2(200, 55))) {

                    // 로그아웃 패킷 전송 로직...
                    auto serverSession = CServerSessionManager::GetInstance().GetServerSession();
                    if (serverSession) {
                        C_LOGOUT logOutPkt;
                        auto user = serverSession->GetUser();
                        if (user) {
                            logOutPkt.user_id = user->GetUserID();
                            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGOUT>(logOutPkt);
                            serverSession->DoSend(sendBuffer);
                        }
                    }
                    
                    // (임시)
                    // is_online = false;
                    //ActionResult result;
                    //result.Success("로그아웃 성공!");
                    //SetLastResult(result);
                }
            }
            ImGui::Spacing(); 
            ImGui::Spacing();

            if (ImGui::Button((const char*)u8"뒤로가기", ImVec2(200, 55))) {
                SetUIState(TitleUIState::Main); // 메인으로 복귀
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void CImGuiManager::DrawSignInWindow()
{
    ImGui::SetNextWindowPos(ImVec2(210, 260), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(390, 220), ImGuiCond_Always);
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    // 창 닫기 감지를 위한 정적 변수 (기본값 true)
    static bool open = true;

    if (ImGui::Begin((const char*)u8"로그인", &open, winFlags)) {
        static char id[64] = "";
        static char pw[64] = "";

        ImGui::Text((const char*)u8"ID / PW 를 입력하세요.");
        ImGui::Separator();

        ImGui::PushItemWidth(230.0f);
        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();

        ImGui::Spacing(); ImGui::Spacing();

        float btnWidth = 380.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        if (ImGui::Button("Connect & Login", ImVec2(btnWidth, 50))) {

            // 패킷 전송
            C_LOGIN loginPkt;
            COPY_STRING(loginPkt.id, id);
            COPY_STRING(loginPkt.password, pw);
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);

            auto serverSession = CServerSessionManager::GetInstance().GetServerSession();
            if (serverSession)
                serverSession->DoSend(sendBuffer);

            // 로딩 시작 및 창 초기화
            StartLoading(LoadingType::Login);
            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));

            // (임시)
            //ActionResult result;
            //result.Success("로그인 성공!");
            //SetPopUpResult(result);
            //StopLoading();
        }
    }
    ImGui::End();

    if (!open) {
        SetUIState(TitleUIState::MultiSelect); // 멀티 선택 메뉴로 복귀
        open = true; // 다음 번에 창이 정상적으로 열리도록 다시 true로 리셋
    }
}

void CImGuiManager::DrawSignUpWindow()
{
    ImGui::SetNextWindowPos(ImVec2(210, 260), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(390, 250), ImGuiCond_Always);
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    static bool open = true;
    if (ImGui::Begin((const char*)u8"회원가입", &open, winFlags)) {
        static char id[64] = "";
        static char pw[64] = "";
        static char name[64] = "";

        ImGui::Text((const char*)u8"정보를 입력하세요.");
        ImGui::Separator();

        ImGui::PushItemWidth(230.0f);
        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
        ImGui::PopItemWidth();

        ImGui::Spacing(); 
        ImGui::Spacing();

        float btnWidth = 380.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        if (ImGui::Button((const char*)u8"가입 신청", ImVec2(btnWidth, 50))) {

            C_SIGNUP signUpPkt;
            COPY_STRING(signUpPkt.id, id);
            COPY_STRING(signUpPkt.password, pw);
            COPY_STRING(signUpPkt.name, name);
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_SIGNUP>(signUpPkt);

            auto session = CServerSessionManager::GetInstance().GetServerSession();
            if (session) 
                session->DoSend(sendBuffer);

            StartLoading(LoadingType::SignUp);

            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));
            memset(name, 0, sizeof(name));
        }
    }
    ImGui::End();

    if (!open) {
        SetUIState(TitleUIState::MultiSelect); // 멀티 선택 메뉴로 복귀
        open = true; // 다음 번에 창이 정상적으로 열리도록 다시 true로 리셋
    }
}

void CImGuiManager::DrawLoadingPopUp()
{
    if (!ImGui::IsPopupOpen("LoadingPopup")) {
        ImGui::OpenPopup("LoadingPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);
        ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

        const char* txt = "로딩 중...";
        switch (loading_type) {
        case LoadingType::Login:      txt = (const char*)u8"로그인 중입니다..."; break;
        case LoadingType::SignUp:     txt = (const char*)u8"가입 처리 중입니다..."; break;
        case LoadingType::RoomCreate: txt = (const char*)u8"방 생성 중입니다..."; break;
        case LoadingType::RoomEnter:  txt = (const char*)u8"방 입장 중입니다..."; break;
        case LoadingType::SinglePlay: txt = (const char*)u8"싱글 플레이 입장 중..."; break;
        }
        ImGui::Text("%s", txt);

        ImGui::Spacing();
        if (ImGui::Button("Cancel")) {
            StopLoading();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CImGuiManager::ShowResultPopup(bool is_success, const std::string& msg)
{
    pop_up_result.is_visible = true;
    pop_up_result.is_success = is_success;
    pop_up_result.message = msg;
}

void CImGuiManager::CloseResultPopup()
{
}

void CImGuiManager::DrawLoadingPopUpResult()
{
    if (!ImGui::IsPopupOpen("ResultPopup")) {
        ImGui::OpenPopup("ResultPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ResultPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", CP949ToUTF8(pop_up_result.message).c_str());
        ImGui::Spacing();

        if (ImGui::Button((const char*)u8"확인")) {

            pop_up_result.is_visible = false; // 팝업 닫기
            ImGui::CloseCurrentPopup();

            // 성공 시 후속 처리 (예: 로그인 성공했으면 방 목록으로)
            if (pop_up_result.is_success) {

                // 어떤 작업이 성공했는지에 따라 분기 가능
                // 현재 로직상 로그인 성공이면 RoomList로 보내는 게 자연스러움

                if (ui_state == TitleUIState::SignUp) {
                    SetUIState(TitleUIState::MultiSelect); // 가입 성공하면 로그인하러 가라
                }
                else if (ui_state == TitleUIState::Login) {
                    is_online = true;
                    is_title_draw = false;
                    SetUIState(TitleUIState::RoomList);
                }
                else if (ui_state == TitleUIState::MultiSelect) {
                    is_online = false;
                }
            }
        }
        ImGui::EndPopup();
    }
}

void CImGuiManager::DrawRoomListUI()
{
    // 전체 배경 (파란 틴트)
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0, 0), ImGui::GetIO().DisplaySize,
        ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.4f)));

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("MatchingWindow", NULL, flags)) {
        // 타이틀 (빨간 UNDEAD)
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        if (title_font2) ImGui::PushFont(title_font2);

        const char* titleText = "UNDEAD";
        float textWidth = ImGui::CalcTextSize(titleText).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text(titleText);
        ImGui::PopStyleColor();
        if (title_font2) ImGui::PopFont();

        ImGui::Spacing(); ImGui::Spacing();

        // 테이블 및 버튼
        DrawRoomListTable();
        DrawRefreshButton();
        DrawThreeButton();

        // 빈 곳 클릭 시 선택 해제
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            selected_room_id = 0;
        }
    }
    ImGui::End();
}

void CImGuiManager::DrawRoomListTable()
{
    float windowWidth = ImGui::GetWindowSize().x;
    float tableWidth = 700.0f; // 비율 조정 필요 시 적용
    float tableHeight = 300.0f;

    ImGui::SetCursorPosX((windowWidth - tableWidth) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f, 0.9f, 0.9f, 0.7f));

    if (ImGui::BeginChild("TableArea", ImVec2(tableWidth, tableHeight), true)) {
        if (ImGui::BeginTable("RoomTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {

            ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn((const char*)u8"방 제목", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn((const char*)u8"인원", ImGuiTableColumnFlags_WidthFixed, 100.0f);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor(2);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 내용 검정색
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 선택 시 노란색

            int rowNum = 1;
            for (const auto& room : room_vec) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", rowNum++);

                std::string utf8Name = CP949ToUTF8(room.room_name);
                std::string uniqueLabel = utf8Name + "##" + std::to_string(room.room_id);

                ImGui::TableSetColumnIndex(1);
                bool is_selected = (selected_room_id == room.room_id);
                if (ImGui::Selectable(uniqueLabel.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_room_id = room.room_id;
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    selected_room_id = room.room_id;
                    StartLoading(LoadingType::RoomEnter);
                    // SendEnterRoomPacket(selected_room_id); 
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d / %d", room.total_player, room.max_player);
            }
            ImGui::PopStyleColor(2);
            ImGui::EndTable();
        }

        // ===============================
        // 테이블 영역 빈 곳 클릭 시 선택 해제
        // ===============================
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            selected_room_id = 0;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void CImGuiManager::DrawRefreshButton()
{
    float windowWidth = ImGui::GetWindowSize().x;
    float tableWidth = 700.0f;
    float btnSize = 60.0f;

    ImGui::SetCursorPosX((windowWidth + tableWidth) * 0.5f - btnSize);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button((const char*)u8"새로\n고침", ImVec2(btnSize, btnSize))) {
        // 새로고침 패킷 전송
    }
    ImGui::PopStyleColor(2);
}

void CImGuiManager::DrawThreeButton()
{
    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    float btnWidth = 200.0f;
    float btnHeight = 60.0f;
    float spacing = 50.0f;
    float totalWidth = (btnWidth * 3) + (spacing * 2);

    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95f, 0.55f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    if (ImGui::Button((const char*)u8"방 만들기", ImVec2(btnWidth, btnHeight))) {
        show_room_create_popup = true;
    }
    ImGui::SameLine(0, spacing);

    if (ImGui::Button((const char*)u8"방 입장", ImVec2(btnWidth, btnHeight))) {
        if (selected_room_id != 0) {
            StartLoading(LoadingType::RoomEnter);
            // 입장 패킷 전송...
        }
    }
    ImGui::SameLine(0, spacing);

    if (ImGui::Button((const char*)u8"뒤로 가기", ImVec2(btnWidth, btnHeight))) {
        is_title_draw = true;
        SetUIState(TitleUIState::MultiSelect); // 다시 메뉴 선택으로
    }

    ImGui::PopStyleColor(2);
}

void CImGuiManager::DrawRoomCreatePopUp()
{
    if (!ImGui::IsPopupOpen("CreateRoom")) {
        ImGui::OpenPopup("CreateRoom");
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("CreateRoom", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char roomName[128] = "";

        ImGui::Text((const char*)u8"생성할 방 제목을 입력하세요.");
        ImGui::Spacing();
        ImGui::PushItemWidth(300.0f);
        ImGui::InputText("##RoomName", roomName, IM_ARRAYSIZE(roomName));
        ImGui::PopItemWidth();
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        if (ImGui::Button((const char*)u8"생성", ImVec2(120, 40))) {
            static int roomCounter = 1;
            if (strlen(roomName) == 0) {
                sprintf_s(roomName, sizeof(roomName), "Unknown Room %d", roomCounter++);
            }

            // 패킷 전송
            C_CreateRoom createPkt;
            auto serverSession = CServerSessionManager::GetInstance().GetServerSession();
            if (serverSession) {
                auto user = serverSession->GetUser();
                if (user) {
                    createPkt.user_id = user->GetUserID();
                    COPY_STRING(createPkt.room_name, roomName);
                    auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_CreateRoom>(createPkt);
                    serverSession->DoSend(sendBuffer);
                }
            }

            memset(roomName, 0, sizeof(roomName));
            ImGui::CloseCurrentPopup();

            show_room_create_popup = false;
            StartLoading(LoadingType::RoomCreate);
        }
        ImGui::SameLine();

        if (ImGui::Button((const char*)u8"취소", ImVec2(120, 40))) {
            memset(roomName, 0, sizeof(roomName));
            ImGui::CloseCurrentPopup();
            show_room_create_popup = false;
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
}

void CImGuiManager::LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count, const float speed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 10.0f;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return;

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

// 헬퍼 함수 구현
std::string CP949ToUTF8(const std::string& strCP949)
{
    if (strCP949.empty()) return "";
    int nwLen = MultiByteToWideChar(949, 0, strCP949.c_str(), -1, NULL, 0);
    wchar_t* pwBuf = new wchar_t[nwLen];
    MultiByteToWideChar(949, 0, strCP949.c_str(), -1, pwBuf, nwLen);
    int nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, 0, NULL, NULL);
    char* pBuf = new char[nLen];
    WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, pBuf, nLen, NULL, NULL);
    std::string strUTF8(pBuf);
    delete[] pwBuf;
    delete[] pBuf;
    return strUTF8;
}
