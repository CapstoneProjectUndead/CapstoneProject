#pragma once
#include "Scene.h"

// 메인 UI 화면 상태 (한 번에 하나만 보임)
enum class TitleUIState
{
    None,               // UI 없음 (인게임 등)
    Main,               // 초기 화면 [싱글][멀티][나가기]
    MultiSelect,        // 멀티 메뉴 [로그인][가입] or [방검색][로그아웃]
    Login,              // 로그인 입력 창
    SignUp,             // 회원가입 입력 창
    RoomList            // 방 목록 (매칭 화면)
};


class CTitleScene :
    public CScene
{
public:
    CTitleScene();
    ~CTitleScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Update(float elapsedTime) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;

    virtual void Enter();
    virtual void Exit();

    virtual void DrawUI() override;

public:

    bool IsUIInputEnabled();

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
    std::unordered_map<uint32, RoomInfo>& GetRooms() { return rooms; }
    int GetSelectedRoomID() { return selected_room_id; }
    void SetSelectedRoomID(int id) { selected_room_id = id; }

private:
    // UI 그리기 함수들
    void DrawTitle();
    void DrawTitleUI();

    void DrawTitleMainWindow();     // 메인/멀티선택 메뉴 통합
    void DrawLogInWindow();
    void DrawSignUpWindow();
    void DrawRoomListUI();          // 방 목록 전체
    void DrawRoomListTable();       // 테이블 부분
    void DrawRoomCreatePopUp();     // 방 생성 팝업

    void DrawLoadingPopUp();        // 로딩 (뺑글이)
    void DrawLoadingPopUpResult();  // 결과 확인 창

    void DrawRefreshButton();
    void DrawThreeButton();

private:
    std::unordered_map<uint32, RoomInfo> rooms;

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

