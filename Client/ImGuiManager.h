#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_internal.h"

// 헬퍼 함수 
// 인자: (텍스처 핸들(ptr), 버튼 텍스트, 버튼 크기)
bool ImageButtonWithText(long long texturePtr, const char* label, const ImVec2& size);


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

private:
	void LoadingIndicatorCircle(const char* label, const float indicator_radius
		, const ImVec4& main_color, const ImVec4& backdrop_color
		, const int circle_count, const float speed);

	void DrawLogInUI();
	void DrawTitle();

private:
    // DX12는 ImGui 폰트용 힙이 꼭 필요합니다.
    ID3D12DescriptorHeap* m_pSrvDescHeap = nullptr;

	ImFont* title_font = nullptr;
};

