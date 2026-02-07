#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"


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
	void DrawLogInUI();

private:
    // DX12는 ImGui 폰트용 힙이 꼭 필요합니다.
    ID3D12DescriptorHeap* m_pSrvDescHeap = nullptr;
};

