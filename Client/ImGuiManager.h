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
std::string UTF8ToCP949(const std::string& utf8Str);


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
    static ImFont* vineritc_font;
    static ImFont* elephnt_font;
    static ImFont* bold_font; 
    static ImFont* creepster_font; 
    static bool need_reset_focus;

public:
    void Init(HWND hwnd, ID3D12Device* device, int numFramesInFlight, DXGI_FORMAT rtvFormat);
    void Update();
    void Render(ID3D12GraphicsCommandList* cmdList);
    void Release();

    static void ResetIMEState(HWND hwnd);
    static void DisableIME(HWND hwnd);
    static void EnableIME(HWND hwnd);
    static void ClearFocus(HWND hwnd);
    static void ReserveResetFocus() { need_reset_focus = true; }

    static void LoadingIndicatorCircle(const char* label, const float indicator_radius
        , const ImVec4& main_color, const ImVec4& backdrop_color
        , const int circle_count, const float speed);

private:
    ID3D12DescriptorHeap*   srv_desc_heap = nullptr;
    static HIMC             default_IMC; // IME 핸들 저장
    
};