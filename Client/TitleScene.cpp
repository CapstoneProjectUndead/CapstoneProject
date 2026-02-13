#include "stdafx.h"
#include "TitleScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "ServerSessionManager.h"
#include "SceneManager.h"
#include "User.h"
#include "ImGuiManager.h"

#define ROOM_MAX_PLAYER 4


CTitleScene::CTitleScene()
	: CScene(SCENE_TYPE::TITLE)
{
}

CTitleScene::~CTitleScene()
{
}

void CTitleScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{

}

void CTitleScene::Update(float elapsedTime)
{
    
}

void CTitleScene::Render(ID3D12GraphicsCommandList*)
{
}

void CTitleScene::Enter()
{
	if (my_player)
		my_player->SetCurrentSceneType(SCENE_TYPE::TITLE);
}

void CTitleScene::Exit()
{
}

void CTitleScene::DrawUI()
{
    // =======================
    // [IME 및 포커스 관리 로직]
    // =======================
    static bool lastInputState = false;
    bool currentInputState = IsUIInputEnabled();
    HWND hwnd = ghWnd;

    // 상태 변경 시에만 IME 제어
    if (currentInputState != lastInputState || need_reset_focus) {

        if (currentInputState) {
            CImGuiManager::EnableIME(hwnd);
        }
        else {
            CImGuiManager::DisableIME(hwnd);
        }

        if (need_reset_focus) {
            ImGuiContext& g = *GImGui;
            g.ActiveId = 0;
            ImGui::SetWindowFocus(nullptr);
            ImGui::GetIO().InputQueueCharacters.resize(0);
            ImGui::GetIO().ClearInputKeys();
            CImGuiManager::ResetIMEState(hwnd);

            need_reset_focus = false;
        }

        lastInputState = currentInputState;
    }

    // UI 그리기 시작
    DrawTitleUI();
}

// =====================
// [IME 제어 헬퍼 함수들]
// =====================
bool CTitleScene::IsUIInputEnabled()
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
    if ((ui_state == TitleUIState::Login))
        state = false;

    return state;
}

// ===================
// [UI 그리기 메인 로직]
// ===================
void CTitleScene::DrawTitleUI()
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
        DrawLogInWindow();
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

void CTitleScene::DrawTitle()
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
        if (CImGuiManager::title_font) ImGui::PushFont(CImGuiManager::title_font);
        const char* titleText = "UNDEAD";
        float textWidth = ImGui::CalcTextSize(titleText).x;
        float windowWidth = ImGui::GetWindowSize().x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
        ImGui::Text(titleText);
        ImGui::PopStyleColor();

        if (CImGuiManager::title_font) ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// 메인 메뉴 & 멀티 선택 메뉴 통합 관리
void CTitleScene::DrawTitleMainWindow()
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
                
                    if (SERVER_SESSION) {
                        auto user = SERVER_SESSION->GetUser();
                        if (user) {
                            C_UpdateRoom updateRoomPkt;
                            auto sendBuffer = MAKE_SEND_BUFFER(updateRoomPkt);
                            SERVER_SESSION->DoSend(sendBuffer);
                        }
                    }
                }
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Button((const char*)u8"로그아웃", ImVec2(200, 55))) {

                    StartLoading(LoadingType::Logout);

                    // 로그아웃 패킷 전송 로직...
                    auto serverSession = GET_SERVER_SESSION
                    if (serverSession) {
                        C_LOGOUT logOutPkt;
                        auto user = serverSession->GetUser();
                        if (user) {
                            logOutPkt.user_id = user->GetUserID();
                            auto sendBuffer = MAKE_SEND_BUFFER(logOutPkt);
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

void CTitleScene::DrawLogInWindow()
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

void CTitleScene::DrawSignUpWindow()
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

void CTitleScene::DrawLoadingPopUp()
{
    if (!ImGui::IsPopupOpen("LoadingPopup")) {
        ImGui::OpenPopup("LoadingPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        CImGuiManager::LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);
        ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

        const char* txt = "로딩 중...";
        switch (loading_type) {
        case LoadingType::Login:      txt = (const char*)u8"로그인 중입니다..."; break;
        case LoadingType::Logout:      txt = (const char*)u8"로그아웃 중입니다..."; break;
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

void CTitleScene::ShowResultPopup(bool is_success, const std::string& msg)
{
    pop_up_result.is_visible = true;
    pop_up_result.is_success = is_success;
    pop_up_result.message = msg;
}

void CTitleScene::CloseResultPopup()
{
    pop_up_result.is_visible = false;
    pop_up_result.is_success = false;
}

void CTitleScene::DrawLoadingPopUpResult()
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
                else if (ui_state == TitleUIState::RoomList) {

                    if (is_room_enter) {
                        is_room_enter = false;

                        CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);                 
                        auto player = CSceneManager::GetInstance().GetActiveScene()->GetMyPlayer();
                        player->SetSession(SERVER_SESSION);
                        player->SetUser(SERVER_SESSION->GetUser());
                        player->SetRoomID(SERVER_SESSION->GetUser()->GetRoomID());
                    }
                }

                CloseResultPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void CTitleScene::DrawRoomListUI()
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
        if (CImGuiManager::title_font2) ImGui::PushFont(CImGuiManager::title_font2);

        const char* titleText = "UNDEAD";
        float textWidth = ImGui::CalcTextSize(titleText).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text(titleText);
        ImGui::PopStyleColor();
        if (CImGuiManager::title_font2) ImGui::PopFont();

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

void CTitleScene::DrawRoomListTable()
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
            for (const auto& [id, room] : rooms) {
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
                ImGui::Text("%d / %d", room.current_player_count, ROOM_MAX_PLAYER);
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

void CTitleScene::DrawRefreshButton()
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
        if (SERVER_SESSION) {
            auto user = SERVER_SESSION->GetUser();
            if (user) {
                C_UpdateRoom updateRoomPkt;
                auto sendBuffer = MAKE_SEND_BUFFER(updateRoomPkt);
                SERVER_SESSION->DoSend(sendBuffer);
            }
        }
    }
    ImGui::PopStyleColor(2);
}

void CTitleScene::DrawThreeButton()
{
    ImGui::Spacing(); 
    ImGui::Spacing(); 
    ImGui::Spacing();

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
            if (SERVER_SESSION) {
                auto user = SERVER_SESSION->GetUser();
                if (user) {
                    C_EnterRoom enterPkt;
                    enterPkt.room_id = selected_room_id;
                    enterPkt.user_id = user->GetUserID();
                    auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
                    SERVER_SESSION->DoSend(sendBuffer);
                }
            }
        }
    }
    ImGui::SameLine(0, spacing);

    if (ImGui::Button((const char*)u8"뒤로 가기", ImVec2(btnWidth, btnHeight))) {
        is_title_draw = true;
        SetUIState(TitleUIState::MultiSelect); // 다시 메뉴 선택으로
    }

    ImGui::PopStyleColor(2);
}

void CTitleScene::DrawRoomCreatePopUp()
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
            if (strlen(roomName) == 0) {
                sprintf_s(roomName, sizeof(roomName), "Unknown Room");
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