#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_internal.h"

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

	void SetTitleDraw(bool result) { is_title_draw = result; }
	void SetLoginLoading(bool loading) { is_login_loading = loading; }
	void SetSignupLoading(bool loading) { is_signup_loading = loading; }
	void SetSignUpResult(bool result) { is_signup_success = result; }
	void SetSignInResult(bool result) { is_signin_success = result; }

	// 필요하다면 창을 강제로 닫는 기능
	void CloseAllWindow() { show_login_window = false; show_sign_window = false; }

private:
	void LoadingIndicatorCircle(const char* label, const float indicator_radius
		, const ImVec4& main_color, const ImVec4& backdrop_color
		, const int circle_count, const float speed);

	void DrawLogInUI();

	void DrawTitleUI();
	void DrawTitle();
	void DrawLoadingPopUp();
	void DrawLoadingPopUpResult();

	void DrawRoomListUI();


private:
    // DX12는 ImGui 폰트용 힙이 꼭 필요합니다.
    ID3D12DescriptorHeap* srv_desc_heap = nullptr;
	std::vector<RoomListInfo> room_vec;

	ImFont* title_font = nullptr;
	ImFont* title_font2 = nullptr;

	bool is_title_draw = true;
	bool show_login_window = false;	// 로그인 입력창 띄우기
	bool show_sign_window = false;	// 회원가입창 띄우기

	bool is_login_loading = false;	// 로딩 팝업 띄우기
	bool is_signup_loading = false; // 로딩 팝업 띄우기

	bool is_multi_signin = false;
	bool is_signup_success = false; // 회원가입 성공여부
	bool is_signin_success = false; // 로그인 성공여부

	bool show_room_list_window = false;	// 룸매칭 화면 뜨위기

	uint16 selected_room_id = -1;
};