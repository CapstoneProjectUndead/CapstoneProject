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
    static ImFont* gyeonggi_font;
    static ImFont* gyeonggi_batang_font;
    static bool need_reset_focus;

public:
    void Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, int numFramesInFlight, DXGI_FORMAT rtvFormat);
    void Update();
    void Render(ID3D12GraphicsCommandList* cmdList);
    void Release();

    // 4월 3일 추가
    // PNG/JPG 등 WIC 포맷 텍스처를 ImGui 힙에 로드. name은 이후 GetTexture()로 참조할 키.
    void LoadTexture(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                     const std::string& name, const std::wstring& path);

    // 로드된 텍스처의 ImTextureID 반환. 없으면 0.
    ImTextureID GetTexture(const std::string& name) const;

    static void ResetIMEState(HWND hwnd);
    static void DisableIME(HWND hwnd);
    static void EnableIME(HWND hwnd);
    static void ClearFocus(HWND hwnd);
    static void ReserveResetFocus() { need_reset_focus = true; }

    static void LoadingIndicatorCircle(const char* label, const float indicator_radius
        , const ImVec4& main_color, const ImVec4& backdrop_color
        , const int circle_count, const float speed);

    // 라운드 타이머 오버레이 (화면 상단 중앙 MM:SS)
    static void DrawRoundTimer(float remainSec);

private:
    ID3D12DescriptorHeap*   srv_desc_heap = nullptr;
    static HIMC             default_IMC; // IME 핸들 저장

    // 4월 3일 추가
    // 유저 텍스처 힙 관리
    static const UINT       MAX_DESCRIPTORS = 128;   // UI 텍스처 수(현재 93)보다 여유있게. 초과 시 힙 범위 밖 SRV 생성으로 아이콘 깨짐
    UINT                    descriptor_size = 0;
    UINT                    next_slot       = 0;

    struct UITexture 
    {
        ComPtr<ID3D12Resource> resource;
        ImTextureID            tex_id = 0;
    };

    std::unordered_map<std::string, UITexture> textures;

    void                          AllocDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu);
    D3D12_CPU_DESCRIPTOR_HANDLE   GetCPUHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE   GetGPUHandle(UINT index) const;
};