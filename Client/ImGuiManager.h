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
    void Update(); // 매 프레임 시작 (Tick)
    void Render(ID3D12GraphicsCommandList* cmdList); // 그리기 명령
    void Shutdown();
	bool IsUIInputEnabled();
	void ResetIMEState(HWND hwnd);

	// 게임 모드: 한글 입력기 완전 제거
	void DisableIME(HWND hwnd)
	{
		// 이미 제거된 상태면 패스
		if (default_imc != nullptr) return;

		// 현재 윈도우의 IME 연결을 끊고, 원래 핸들을 저장해둠 (나중에 복구용)
		default_imc = ImmAssociateContext(hwnd, NULL);

		printf("[System] IME Disabled (Game Mode)\n");
	}

	// UI 모드: 한글 입력기 복구
	void EnableIME(HWND hwnd)
	{
		// 저장해둔 핸들이 없으면 패스
		if (default_imc == nullptr) return;

		// IME 다시 연결
		ImmAssociateContext(hwnd, default_imc);
		default_imc = nullptr;

		printf("[System] IME Enabled (UI Mode)\n");
	}

	void SetTitleDraw(bool result) { is_title_draw = result; }

	void SetLoginLoading(bool loading) { is_login_loading = loading; }
	void SetSignupLoading(bool loading) { is_signup_loading = loading; }
	void SetRoomCreateLoading(bool loading) { is_room_create_loading = loading; }
	void SetRoomEnterLoading(bool loading) { is_room_enter_loading = loading; }

	void SetSignUpAlarm(bool alarm) { signup_alarm = alarm; }
	void SetSignUpResult(bool result) { is_signup_success = result; }
	void SetSignInResult(bool result) { is_signin_success = result; }
	void SetIsOnlie(bool result) { is_online = result; }

	// 필요하다면 창을 강제로 닫는 기능
	void CloseAllWindow() { show_login_window = false; show_sign_window = false; }
	
	void Reset()
	{
		show_login_window = show_sign_window = is_login_loading = is_signup_loading = false;
	}

	void ReserveResetFocus() { need_reset_focus = true; }

	void LoadingIndicatorCircle(const char* label, const float indicator_radius
		, const ImVec4& main_color, const ImVec4& backdrop_color
		, const int circle_count, const float speed);

private:
	//===================
	// 안쓰는 함수 (참고용)
	void DrawLogInUI();
	//===================

	void DrawTitle();
	void DrawTitleUI();

	void DrawTitleMainWindow();
	void DrawFirstMenuButton(bool& menu);
	void DrawSecondMenuButton(bool& menu);
	void DrawThirdMenuButton(bool& menu);

	void DrawSignInWidow();
	void DrawSignUpWindow();

	void DrawLoadingPopUp();
	void DrawLoadingPopUpResult();

	void DrawRoomListUI();
	void DrawRoomListMainWindow();
	void DrawRoomListTable();
	void DrawRefreshButton();
	void DrawThreeButton();	// 방 만들기 / 방 입장 / 뒤로가기

	void DrawRoomCreatePopUp();

private:
    // DX12는 ImGui 폰트용 힙이 꼭 필요합니다.
    ID3D12DescriptorHeap* srv_desc_heap = nullptr;
	HIMC				  default_imc = nullptr;

	std::vector<RoomInfo> room_vec;

	ImFont* title_font = nullptr;
	ImFont* title_font2 = nullptr;

	bool need_reset_focus = false;

	bool is_title_draw = true;
	bool show_login_window = false;	// 로그인 입력창 띄우기
	bool show_sign_window = false;	// 회원가입창 띄우기

	bool is_single_loading = false; // 로딩 팝업 띄우기
	bool is_login_loading = false;	// 로딩 팝업 띄우기
	bool is_signup_loading = false; // 로딩 팝업 띄우기
	bool is_room_create_loading = false; // 로딩 팝업 띄우기
	bool is_room_enter_loading = false; // 로딩 팝업 띄우기

	bool signup_alarm = false;
	bool is_signup_success = false; // 회원가입 성공여부
	bool is_signin_success = false; // 로그인 성공여부
	bool is_online = false;			// 로그인 여부

	bool show_room_list_window = false;	// 룸매칭 화면 뜨위기

	uint16 selected_room_id = -1;
};