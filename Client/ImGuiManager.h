#pragma once
#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>
#include <ImGui/imgui_internal.h>

#include <imm.h>
#pragma comment(lib, "imm32.lib")

// 헬퍼 함수 
// 인자: (텍스처 핸들(ptr), 버튼 텍스트, 버튼 크기)
bool ImageButtonWithText(long long texturePtr, const char* label, const ImVec2& size);
std::string CP949ToUTF8(const std::string& strCP949);

// 1. 메인 UI 화면 상태 (한 번에 하나만 보임)
enum class TitleUIState
{
    None,               // UI 없음 (인게임 등)
    Main,               // 초기 화면 [싱글][멀티][나가기]
    MultiSelect,        // 멀티 메뉴 [로그인][가입] or [방검색][로그아웃]
    Login,              // 로그인 입력 창
    SignUp,             // 회원가입 입력 창
    RoomList            // 방 목록 (매칭 화면)
};

// 2. 로딩 팝업 종류
enum class LoadingType
{
    None,
    Login,
    SignUp,
    RoomCreate,
    RoomEnter,
    SinglePlay
};

// 3. 결과 팝업 데이터 (성공/실패 메시지)
struct ActionResult
{
    bool is_visible = false;
    bool is_success = false; 
    std::string message;

    void Success(const std::string& _message)
    {
        is_visible = true;
        is_success = true;
        message = _message;
    }

    void Fail(const std::string& _message)
    {
        is_visible = true;
        is_success = false;
        message = _message;
    }
};

// 헬퍼 함수
bool ImageButtonWithText(long long texturePtr, const char* label, const ImVec2& size);
std::string CP949ToUTF8(const std::string& strCP949);

class CImGuiManager
{
private:
    CImGuiManager();
    CImGuiManager(const CImGuiManager&) = delete;

public:
    ~CImGuiManager();

    static CImGuiManager& GetInstance() {
        static CImGuiManager instance;
        return instance;
    }

public:
    void Init(HWND hwnd, ID3D12Device* device, int numFramesInFlight, DXGI_FORMAT rtvFormat);
    void Update();
    void Render(ID3D12GraphicsCommandList* cmdList);
    void Shutdown();

    // IME 및 입력 제어 관련
    bool IsUIInputEnabled();
    void ResetIMEState(HWND hwnd);
    void DisableIME(HWND hwnd);
    void EnableIME(HWND hwnd);
    void ClearFocus(HWND hwnd);
    void ReserveResetFocus() { need_reset_focus = true; }

    // -----------------------------------------------------
    // 상태 변경 함수들 (외부에서 호출)
    // -----------------------------------------------------
    void SetUIState(TitleUIState state) { ui_state = state; }
    TitleUIState GetUIState() const { return ui_state; }

    void StartLoading(LoadingType type) { loading_type = type; }
    void StopLoading() { loading_type = LoadingType::None; }

    void SetPopUpResult(const ActionResult& result) { pop_up_result = result; }

    void ShowResultPopup(bool is_success, const std::string& msg);
    void CloseResultPopup();

    void SetOnline(bool online) { is_online = online; }
    void SetRoomCreatePopup(bool show) { show_room_create_popup = show; }

    // 데이터 갱신
    std::vector<RoomInfo>& GetRoomVec() { return room_vec; }
    int GetSelectedRoomID() { return selected_room_id; }
    void SetSelectedRoomID(int id) { selected_room_id = id; }

    static void LoadingIndicatorCircle(const char* label, const float indicator_radius
        , const ImVec4& main_color, const ImVec4& backdrop_color
        , const int circle_count, const float speed);

private:
    // UI 그리기 함수들
    void DrawTitle();
    void DrawTitleUI();

    void DrawTitleMainWindow();     // 메인/멀티선택 메뉴 통합
    void DrawSignInWindow();
    void DrawSignUpWindow();
    void DrawRoomListUI();          // 방 목록 전체
    void DrawRoomListTable();       // 테이블 부분
    void DrawRoomCreatePopUp();     // 방 생성 팝업

    void DrawLoadingPopUp();        // 로딩 (뺑글이)
    void DrawLoadingPopUpResult();  // 결과 확인 창

    void DrawRefreshButton();
    void DrawThreeButton();

private:
    ID3D12DescriptorHeap* srv_desc_heap = nullptr;
    HIMC m_hDefaultIMC = nullptr; // IME 핸들 저장

    std::vector<RoomInfo> room_vec;
    ImFont* title_font = nullptr;
    ImFont* title_font2 = nullptr;

    // -----------------------------------------------------
    // [핵심] 리팩토링된 상태 변수들
    // -----------------------------------------------------
    TitleUIState ui_state = TitleUIState::Main;
    LoadingType loading_type = LoadingType::None;
    ActionResult pop_up_result;

    bool is_online = false;
    bool is_title_draw = true;
    bool need_reset_focus = false;
    bool show_room_create_popup = false;
    int  selected_room_id = 0;
};