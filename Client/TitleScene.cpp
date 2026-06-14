#include "stdafx.h"
#include "TitleScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "SceneManager.h"
#include "User.h"
#include "SoundManager.h"

#define ROOM_MAX_PLAYER 4

CTitleScene::CTitleScene()
    : CScene(SCENE_TYPE::TITLE)
{
}

CTitleScene::~CTitleScene()
{
}

void CTitleScene::Initialize()
{

}

void CTitleScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{

}

void CTitleScene::Update(float elapsedTime)
{
    CScene::Update(elapsedTime);
}

void CTitleScene::Render(ID3D12GraphicsCommandList*)
{
}

void CTitleScene::Enter()
{
    CScene::Enter();
    if (my_player)
        my_player->SetCurrentSceneType(SCENE_TYPE::TITLE);
}

void CTitleScene::DrawUI()
{
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
    assert(scene);

    // 타이틀 씬이면 무조건 입력 허용
    if (scene->GetSceneType() == SCENE_TYPE::TITLE)
        state = true;
    else
        state = false;

    // 로그인할 때는 한글 입력 차단
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

    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // 배경 이미지 (없으면 단색 폴백)
    ImTextureID bgTex = CImGuiManager::GetInstance().GetTexture("bg");
    if (bgTex) {
        ImGui::GetBackgroundDrawList()->AddImage(bgTex, ImVec2(0, 0), screenSize);
    }
    else {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(0, 0), screenSize, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 1.0f))
        );
    }

    // 타이틀 이미지 (없으면 기존 폰트 텍스트 폴백)
    ImTextureID titleTex = CImGuiManager::GetInstance().GetTexture("title");
    if (titleTex) {
        float titleW = 600.0f * G_RATIO_X;
        float titleH = 200.0f * G_RATIO_Y;
        float titleX = (screenSize.x - titleW) * 0.5f;
        float titleY = screenSize.y * 0.3f - titleH * 0.5f;
        ImGui::GetBackgroundDrawList()->AddImage(
            titleTex,
            ImVec2(titleX, titleY),
            ImVec2(titleX + titleW, titleY + titleH)
        );
    }
    else if (CImGuiManager::creepster_font) {
        float fontSize = 270.0f * G_RATIO_Y;
        ImFont* font = CImGuiManager::creepster_font;
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "UNDEAD");
        ImVec2 pos = ImVec2(
            (screenSize.x - textSize.x) * 0.5f,
            screenSize.y * 0.3f - textSize.y * 0.5f
        );
        float s = G_RATIO_Y;
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x + 22.0f * s, pos.y + 22.0f * s), IM_COL32( 60, 20,  90,  80), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x + 18.0f * s, pos.y + 18.0f * s), IM_COL32( 80, 30, 110, 100), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x + 14.0f * s, pos.y + 14.0f * s), IM_COL32(100, 40, 140, 130), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x + 10.0f * s, pos.y + 10.0f * s), IM_COL32(130, 60, 170, 155), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x +  6.0f * s, pos.y +  6.0f * s), IM_COL32(155, 80, 200, 180), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, ImVec2(pos.x +  3.0f * s, pos.y +  3.0f * s), IM_COL32(180, 100, 220, 210), "UNDEAD");
        ImGui::GetBackgroundDrawList()->AddText(font, fontSize, pos, IM_COL32(100, 220, 80, 255), "UNDEAD");
    }
}

// 메인 메뉴 & 멀티 선택 메뉴 통합 관리
void CTitleScene::DrawTitleMainWindow()
{
    // 1. 현재 화면 해상도 가져오기
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    float scale = G_RATIO_Y;

    // 2. 버튼의 가로/세로 크기 모두에 scale을 곱한다.
    ImVec2 btnSize = ImVec2(200.0f * scale, 55.0f * scale);

    // 위치는 화면 비율(%) 기반이므로 그대로 둡니다. 
    ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.75f);
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags mainBtnFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Main Menu", NULL, mainBtnFlags))
    {
        // 폰트 크기에도 똑같이 scale을 적용. (버튼과 글씨가 일체감 있게 커짐)
        ImGui::SetWindowFontScale(scale);

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
            ImTextureID singleTex = CImGuiManager::GetInstance().GetTexture("single");
            ImTextureID multiTex  = CImGuiManager::GetInstance().GetTexture("multi");
            ImTextureID exitTex   = CImGuiManager::GetInstance().GetTexture("exit");

            if (ImageButtonWithText((long long)singleTex, "##single", btnSize)) {
                // 싱글 모드
                g_is_single = true;
                CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
                PlayClickSound();
            }
            CheckHoverSound();
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImageButtonWithText((long long)multiTex, "##multi", btnSize)) {
                SetUIState(TitleUIState::MultiSelect);
                PlayClickSound();
            }
            CheckHoverSound();
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImageButtonWithText((long long)exitTex, "##exit", btnSize)) {
                g_run = false;
                PlayClickSound();
            }
            CheckHoverSound();
        }
        // =======================
        // [State 2] 멀티 선택 화면
        // =======================
        else if (ui_state == TitleUIState::MultiSelect)
        {
            if (!is_online) {
                // 오프라인 상태: 로그인 / 회원가입 / 뒤로가기
                ImTextureID loginTex    = CImGuiManager::GetInstance().GetTexture("login");
                ImTextureID registerTex = CImGuiManager::GetInstance().GetTexture("register");

                if (ImageButtonWithText((long long)loginTex, "##login", btnSize)) {
                    SetUIState(TitleUIState::Login);
                    PlayClickSound();
                }
                CheckHoverSound();
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImageButtonWithText((long long)registerTex, "##register", btnSize)) {
                    SetUIState(TitleUIState::SignUp);
                    PlayClickSound();
                }
                CheckHoverSound();
            }
            else {
                // 온라인 상태: 방 검색 / 로그아웃
                ImTextureID searchTex = CImGuiManager::GetInstance().GetTexture("search");
                ImTextureID logoutTex = CImGuiManager::GetInstance().GetTexture("logout");

                if (ImageButtonWithText((long long)searchTex, "##search", btnSize)) {
                    is_title_draw = false;
                    SetUIState(TitleUIState::RoomList);
                    PlayClickSound();

                    if (SERVER_SESSION) {
                        auto user = SERVER_SESSION->GetUser();
                        if (user) {
                            C_UpdateRoom updateRoomPkt;
                            auto sendBuffer = MAKE_SEND_BUFFER(updateRoomPkt);
                            SERVER_SESSION->DoSend(sendBuffer);
                        }
                    }
                }
                CheckHoverSound();
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImageButtonWithText((long long)logoutTex, "##logout", btnSize)) {

                    StartLoading(LoadingType::Logout);
                    PlayClickSound();

                    // 로그아웃 패킷 전송 로직
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
                CheckHoverSound();
            }
            ImGui::Spacing();
            ImGui::Spacing();

            ImTextureID goBackTex = CImGuiManager::GetInstance().GetTexture("goback");
            if (ImageButtonWithText((long long)goBackTex, "##goback_main", btnSize)) {
                SetUIState(TitleUIState::Main); // 메인으로 복귀
                PlayClickSound();
            }
            CheckHoverSound();
        }

        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void CTitleScene::DrawLogInWindow()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    float scale = G_RATIO_Y;

    ImVec2 winSize = ImVec2(340.0f * scale, 0.0f); // 높이 0 = 콘텐츠에 맞춰 자동(버튼 아래 빈 공간 제거)
    ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.65f);

    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    // 창 닫기 감지를 위한 정적 변수 (기본값 true)
    static bool open = true;

    if (ImGui::Begin((const char*)u8"로그인", &open, winFlags)) {

        ImGui::SetWindowFontScale(scale);

        static char id[64] = "";
        static char pw[64] = "";

        ImGui::Text((const char*)u8"ID / PW 를 입력하세요.");
        ImGui::Separator();

        ImGui::PushItemWidth(230.0f * scale);
        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();

        ImGui::Spacing(); ImGui::Spacing();

        // 버튼 크기 및 중앙 정렬 스케일링 (두 버튼을 한 줄에 나란히)
        float btnWidth = 150.0f * scale;
        float btnHeight = 45.0f * scale;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = btnWidth * 2.0f + spacing;

        // 동적 중앙 정렬: (창 너비 - 두 버튼 전체 너비) / 2
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);

        // 게스트 로그인 (회원가입 없이 바로 입장) — 초록색 버튼
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.26f, 0.63f, 0.28f, 1.0f)); // 기본
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.36f, 0.73f, 0.38f, 1.0f)); // 마우스 오버
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.20f, 0.50f, 0.22f, 1.0f)); // 클릭
        bool guestClicked = ImGui::Button("Guest Login", ImVec2(btnWidth, btnHeight));
        ImGui::PopStyleColor(3);
        if (guestClicked) {
            PlayClickSound();

            // guest_login=true로 전송 → 서버가 ID/PW 검사 없이 입장 처리
            C_LOGIN loginPkt;
            loginPkt.guest_login = true;
            COPY_STRING(loginPkt.id, "");
            COPY_STRING(loginPkt.password, "");
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);

            auto serverSession = CServerSessionManager::GetInstance().GetServerSession();
            if (serverSession)
                serverSession->DoSend(sendBuffer);

            // 로딩 시작 및 창 초기화
            StartLoading(LoadingType::Login);
            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));

            // 폰트 스케일 원상 복구
            ImGui::SetWindowFontScale(1.0f);
        }

        ImGui::SameLine();

        // 일반 로그인 (ID/PW로 DB·파일 인증)
        if (ImGui::Button("Login", ImVec2(btnWidth, btnHeight))) {
            PlayClickSound();

            // 패킷 전송
            C_LOGIN loginPkt;
            loginPkt.guest_login = false;
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

            // 폰트 스케일 원상 복구
            ImGui::SetWindowFontScale(1.0f);
        }
        CheckHoverSound();
    }
    ImGui::End();

    if (!open) {
        SetUIState(TitleUIState::MultiSelect); // 멀티 선택 메뉴로 복귀
        open = true; // 다음 번에 창이 정상적으로 열리도록 다시 true로 리셋
    }
}

void CTitleScene::DrawSignUpWindow()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    float scale = G_RATIO_Y;

    ImVec2 winSize = ImVec2(295.0f * scale, 0.0f); // 높이 0 = 콘텐츠에 맞춰 자동(버튼 아래 빈 공간 제거)
    ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.65f);

    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    static bool open = true;
    if (ImGui::Begin((const char*)u8"회원가입", &open, winFlags)) {

        ImGui::SetWindowFontScale(scale);

        static char id[64] = "";
        static char pw[64] = "";
        static char name[64] = "";

        ImGui::Text((const char*)u8"정보를 입력하세요.");
        ImGui::Separator();

        ImGui::PushItemWidth(230.0f * scale);
        ImGui::InputText("ID", id, IM_ARRAYSIZE(id));
        ImGui::InputText("PW", pw, IM_ARRAYSIZE(pw), ImGuiInputTextFlags_Password);
        ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Spacing();

        // 버튼 크기 및 중앙 정렬 스케일링
        float btnWidth = 250.0f * scale;
        float btnHeight = 50.0f * scale;

        // 동적 중앙 정렬: (현재 창의 실제 너비 - 스케일 적용된 버튼 너비) / 2
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        if (ImGui::Button((const char*)u8"가입 신청", ImVec2(btnWidth, btnHeight))) {
            PlayClickSound();

            // ImGui 입력 문자열은 UTF-8이므로, 서버 규칙(CP949 수신 → to_utf8 변환)에
            // 맞춰 보내기 전에 CP949로 변환한다. (방 생성 코드와 동일 패턴)
            std::string cp949Name = UTF8ToCP949(name);

            C_SIGNUP signUpPkt;
            COPY_STRING(signUpPkt.id, id);
            COPY_STRING(signUpPkt.password, pw);
            COPY_STRING(signUpPkt.name, cp949Name.c_str());
            auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_SIGNUP>(signUpPkt);

            auto session = CServerSessionManager::GetInstance().GetServerSession();
            if (session)
                session->DoSend(sendBuffer);

            StartLoading(LoadingType::SignUp);

            memset(id, 0, sizeof(id));
            memset(pw, 0, sizeof(pw));
            memset(name, 0, sizeof(name));
        }
        CheckHoverSound();

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);
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

    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {

        float scale = G_RATIO_Y;

        CImGuiManager::LoadingIndicatorCircle("spinner", 20.0f * scale, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);
        ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

        ImGui::SetWindowFontScale(scale);

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
            PlayClickSound();
            StopLoading();
            ImGui::CloseCurrentPopup();
        }

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);

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

    if (ImGui::BeginPopupModal("ResultPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {

        float scale = G_RATIO_Y;

        ImGui::SetWindowFontScale(scale);

        ImGui::Text("%s", CP949ToUTF8(pop_up_result.message).c_str());
        ImGui::Spacing();

        if (ImGui::Button((const char*)u8"확인")) {
            PlayClickSound();

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

                        // 서버는 EnterRoom 처리 시 이미 player를 CUSTOMS에 만들어 둠.
                        // 클라는 그 사실에 맞춰 화면만 전환.
                        CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
                    }
                }

                CloseResultPopup();
            }

            // 폰트 스케일 원상 복구
            ImGui::SetWindowFontScale(1.0f);
        }
        ImGui::EndPopup();
    }
}

void CTitleScene::DrawRoomListUI()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // 전체 배경 (bg 텍스처)
    ImTextureID bgTex = CImGuiManager::GetInstance().GetTexture("bg");
    if (bgTex) {
        ImGui::GetBackgroundDrawList()->AddImage(bgTex, ImVec2(0, 0), screenSize);
    }
    else {
        // 폴백: 어두운 회색
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(0, 0), screenSize,
            ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.4f)));
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(screenSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("MatchingWindow", NULL, flags)) {

        float scale = G_RATIO_Y;
        ImGui::SetWindowFontScale(scale);

        // 타이틀 (title 이미지, 크기는 기존 UNDEAD 텍스트와 동일)
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

        // 현재 elephnt_font + SetWindowFontScale 적용된 "UNDEAD" 렌더 크기 계산
        if (CImGuiManager::elephnt_font) ImGui::PushFont(CImGuiManager::elephnt_font);
        ImVec2 titleSize = ImGui::CalcTextSize("UNDEAD");
        if (CImGuiManager::elephnt_font) ImGui::PopFont();

        // 타이틀 이미지 크기 미세 조정 배율 (조금 작게)
        const float titleImgScale = 0.9f;
        ImVec2 titleImgSize = ImVec2(titleSize.x * titleImgScale, titleSize.y * titleImgScale);

        ImTextureID titleTex = CImGuiManager::GetInstance().GetTexture("title");
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleImgSize.x) * 0.5f);
        if (titleTex) {
            ImGui::Image(titleTex, titleImgSize);
        }
        else {
            // 폴백: 기존 텍스트
            if (CImGuiManager::elephnt_font) ImGui::PushFont(CImGuiManager::elephnt_font);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted("UNDEAD");
            ImGui::PopStyleColor();
            if (CImGuiManager::elephnt_font) ImGui::PopFont();
        }

        ImGui::Spacing(); ImGui::Spacing();

        // 테이블 및 버튼
        DrawRoomListTable();
        DrawRefreshButton();
        DrawThreeButton();

        // 빈 곳 클릭 시 선택 해제
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            selected_room_id = 0;
        }

        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();
}

void CTitleScene::DrawRoomListTable()
{
    float scale = G_RATIO_Y;

    float windowWidth = ImGui::GetWindowSize().x;
    float tableWidth = 700.0f * scale; // 비율 조정
    float tableHeight = 300.0f * scale;

    ImGui::SetCursorPosX((windowWidth - tableWidth) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f, 0.9f, 0.9f, 0.7f));

    if (ImGui::BeginChild("TableArea", ImVec2(tableWidth, tableHeight), true)) {

        // 테이블 영역 안의 모든 텍스트(헤더, 내용) 크기를 scale만큼 키웁니다.
        ImGui::SetWindowFontScale(scale);

        if (ImGui::BeginTable("RoomTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {

            // 고정 너비(WidthFixed)로 잡힌 컬럼들도 글자가 커진 만큼 넓혀줍니다.
            // 50.0f, 100.0f 같은 고정 수치에 scale을 곱해야 글자가 밖으로 삐져나가지 않습니다.
            ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 50.0f * scale);
            ImGui::TableSetupColumn((const char*)u8"방 제목", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn((const char*)u8"인원", ImGuiTableColumnFlags_WidthFixed, 100.0f * scale);

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
                    
                    // 더블 클릭했을 때, 방 입장 처리
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

        // 폰트 스케일을 원상 복구
        ImGui::SetWindowFontScale(1.0f);

    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void CTitleScene::DrawRefreshButton()
{
    float scale = G_RATIO_Y;

    float windowWidth = ImGui::GetWindowSize().x;
    float tableWidth = 700.0f * scale;
    float btnSize = 60.0f * scale;

    ImGui::SetCursorPosX((windowWidth + tableWidth) * 0.5f - btnSize);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

    ImTextureID refreshTex = CImGuiManager::GetInstance().GetTexture("refresh");
    if (ImageButtonWithText((long long)refreshTex, "##refresh", ImVec2(btnSize, btnSize))) {
        PlayClickSound();
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
    CheckHoverSound();
}

void CTitleScene::DrawThreeButton()
{
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    float scale = G_RATIO_Y;

    float btnWidth = 200.0f * scale;
    float btnHeight = 60.0f * scale;
    float spacing = 50.0f * scale;
    float totalWidth = (btnWidth * 3) + (spacing * 2);

    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalWidth) * 0.5f);

    ImTextureID createRoomTex = CImGuiManager::GetInstance().GetTexture("createroom");
    ImTextureID enterRoomTex  = CImGuiManager::GetInstance().GetTexture("enterroom");
    ImTextureID goBackTex     = CImGuiManager::GetInstance().GetTexture("goback");

    if (ImageButtonWithText((long long)createRoomTex, "##createroom", ImVec2(btnWidth, btnHeight))) {
        PlayClickSound();
        show_room_create_popup = true;
    }
    CheckHoverSound();
    ImGui::SameLine(0, spacing);

    if (ImageButtonWithText((long long)enterRoomTex, "##enterroom", ImVec2(btnWidth, btnHeight))) {
        if (selected_room_id != 0) {
            PlayClickSound();
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
    CheckHoverSound();
    ImGui::SameLine(0, spacing);

    if (ImageButtonWithText((long long)goBackTex, "##goback", ImVec2(btnWidth, btnHeight))) {
        PlayClickSound();
        is_title_draw = true;
        SetUIState(TitleUIState::MultiSelect); // 다시 메뉴 선택으로
    }
    CheckHoverSound();
}

void CTitleScene::DrawRoomCreatePopUp()
{
    if (!ImGui::IsPopupOpen("CreateRoom")) {
        ImGui::OpenPopup("CreateRoom");
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("CreateRoom", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        float scale = G_RATIO_Y;

        ImGui::SetWindowFontScale(scale);

        static char roomName[128] = "";

        ImGui::Text((const char*)u8"생성할 방 제목을 입력하세요.");
        ImGui::Spacing();
        ImGui::PushItemWidth(250.0f * scale);
        ImGui::InputText("##RoomName", roomName, IM_ARRAYSIZE(roomName));
        ImGui::PopItemWidth();
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImVec2 btnSize = ImVec2(120.0f * scale, 40.0f * scale);

        if (ImGui::Button((const char*)u8"생성", btnSize)) {
            PlayClickSound();
            if (strlen(roomName) == 0) {
                sprintf_s(roomName, sizeof(roomName), "Unknown Room");
            }

            std::string cp949Name = UTF8ToCP949(roomName);

            // 패킷 전송
            C_CreateRoom createPkt;
            auto serverSession = CServerSessionManager::GetInstance().GetServerSession();
            if (serverSession) {
                auto user = serverSession->GetUser();
                if (user) {
                    createPkt.user_id = user->GetUserID();
                    COPY_STRING(createPkt.room_name, cp949Name.c_str());
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

        if (ImGui::Button((const char*)u8"취소", btnSize)) {
            PlayClickSound();
            memset(roomName, 0, sizeof(roomName));
            ImGui::CloseCurrentPopup();
            show_room_create_popup = false;
        }

        ImGui::SetWindowFontScale(1.0f);

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
}

//========================
// 서버 패킷 관련 처리 함수들
//========================
void CTitleScene::Handle_S_Login(std::shared_ptr<Session>& session, const S_LOGIN& pkt)
{
    // 로그인 성공
    if (pkt.success) {

        // 싱글 모드가 아닌 멀티 모드로 전환
        g_is_single = false;

        ShowResultPopup(true, "로그인 성공!");

        // 로딩창 끄기
        StopLoading();

        // User 생성
        std::shared_ptr<CUser> user = std::make_shared<CUser>();

        // session, id 저장 (약한 참조)
        user->SetSession(session);
        user->SetUserID(pkt.user_id);

        // 서버가 보내준 본인 이름 저장
        user->SetName(pkt.name);

        // User Refcount 증가
        SERVER_SESSION->SetUser(user);
    }
    // 로그인 실패
    else {
        CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(false, "로그인 실패!");

        // 로딩창 끄기
        CSceneManager::GetInstance().GetTitleScene()->StopLoading();
    }
}

void CTitleScene::Handle_S_Logout(std::shared_ptr<Session>& session, const S_LOGOUT& pkt)
{
    if (pkt.success) {
        CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "로그아웃 성공!");
        CSceneManager::GetInstance().GetTitleScene()->StopLoading();
        SERVER_SESSION->SetUser(nullptr);
    }
}

void CTitleScene::Handle_S_SignRes(std::shared_ptr<Session>& session, const S_SIGN_RES& pkt)
{
    if (pkt.success) {
        printf("signup success! \n");

        // 가입 성공
        ShowResultPopup(true, "가입 성공!");

        // 로딩창 끄기
        StopLoading();
    }
    else {
        printf("signup fail...! \n");

        // 가입 실패
        ShowResultPopup(false, "가입 실패..!");

        // 로딩창 끄기
        StopLoading();
    }
}

void CTitleScene::Handle_S_EnterRoom(std::shared_ptr<Session>& session, const S_EnterRoom& pkt)
{
    if (pkt.success) {
        SetIsEnter(true);

        ShowResultPopup(true, "방 입장 완료!");

        CImGuiManager::GetInstance().ReserveResetFocus();

        SERVER_SESSION->GetUser()->SetRoomID(pkt.room_id);
    }
    else {
        SetIsEnter(false);

        ShowResultPopup(true, "방 입장 불가!");

        CImGuiManager::GetInstance().ReserveResetFocus();
    }

    // 로딩창 끄기
    StopLoading();
}

void CTitleScene::Handle_S_RoomList(std::shared_ptr<Session> session, S_Room_List& pkt)
{
    S_Room_List::RoomList roomList = pkt.GetRoomList();

    uint32* newRoom = nullptr;
    if (pkt.room_count > 0)
        newRoom = new uint32[pkt.room_count];

    for (int i = 0; i < pkt.room_count; ++i) {
        newRoom[i] = roomList[i].room_info.room_id;

        RoomInfo info{ roomList[i].room_info.room_id, roomList[i].room_info.room_name
            , roomList[i].room_info.current_player_count, roomList[i].room_info.is_game_start };

        auto iter = rooms.find(info.room_id);
        if (iter == rooms.end())
            rooms.insert({ info.room_id, info });
        else
            rooms[info.room_id] = info;
    }

    // 기존에는 있었는데 없어진 방 체크
    if (pkt.room_count == 0) {
        rooms.clear();
    }
    else {
        for (auto it = rooms.begin(); it != rooms.end(); ) {
            int id = it->first;

            bool found = false;
            for (int i = 0; i < pkt.room_count; ++i) {
                if (newRoom[i] == id) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                it = rooms.erase(it); // 삭제 + 다음 iterator 반환
            }
            else {
                ++it;
            }
        }
    }

    if (newRoom != nullptr)
        delete[] newRoom;
}