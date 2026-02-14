#include "stdafx.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ServerSessionManager.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "User.h"

#undef min
#undef max

#define ROOM_MAX_PLAYER 4

HIMC CImGuiManager::m_hDefaultIMC = nullptr;
bool CImGuiManager::need_reset_focus = false;
ImFont* CImGuiManager::title_font = nullptr;
ImFont* CImGuiManager::title_font2 = nullptr;

CImGuiManager::CImGuiManager()
{
}

CImGuiManager::~CImGuiManager()
{
    Release();
}

void CImGuiManager::Init(HWND hwnd, ID3D12Device* device, int numFramesInFlight, DXGI_FORMAT rtvFormat)
{
    // 1. ImGui 컨텍스트 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsLight();

    // 폰트 로드
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgun.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    title_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\VINERITC.TTF", 270.0f);
    title_font2 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ELEPHNT.TTF", 80.0f);
    io.Fonts->Build();

    // Win32 & DX12 초기화
    ImGui_ImplWin32_Init(hwnd);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv_desc_heap));

    ImGui_ImplDX12_Init(device, numFramesInFlight,
        rtvFormat,
        srv_desc_heap,
        srv_desc_heap->GetCPUDescriptorHandleForHeapStart(),
        srv_desc_heap->GetGPUDescriptorHandleForHeapStart());
}

void CImGuiManager::Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    CScene* activeScene = CSceneManager::GetInstance().GetActiveScene();
    if (activeScene) {
        activeScene->DrawUI();
    }
}

void CImGuiManager::Render(ID3D12GraphicsCommandList* cmdList)
{
    ImGui::Render();
    ID3D12DescriptorHeap* ppHeaps[] = { srv_desc_heap };
    cmdList->SetDescriptorHeaps(1, ppHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void CImGuiManager::Release()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (srv_desc_heap) {
        srv_desc_heap->Release();
        srv_desc_heap = nullptr;
    }
}

void CImGuiManager::ResetIMEState(HWND hwnd)
{
    HIMC himc = ImmGetContext(hwnd);
    if (himc) {
        ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmSetOpenStatus(himc, FALSE);
        ImmReleaseContext(hwnd, himc);
    }
}

void CImGuiManager::DisableIME(HWND hwnd)
{
    if (m_hDefaultIMC != nullptr) 
        return;

    m_hDefaultIMC = ImmAssociateContext(hwnd, NULL);
}

void CImGuiManager::EnableIME(HWND hwnd)
{
    if (m_hDefaultIMC == nullptr) 
        return;

    ImmAssociateContext(hwnd, m_hDefaultIMC);
    m_hDefaultIMC = nullptr;
}

void CImGuiManager::ClearFocus(HWND hwnd)
{
    need_reset_focus = true;
    DisableIME(hwnd); // 즉시 차단
}

void CImGuiManager::LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count, const float speed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 10.0f;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return;

    const float t = g.Time;
    const auto degree_offset = 2.0f * IM_PI / circle_count;

    for (int i = 0; i < circle_count; ++i) {
        const auto x = indicator_radius * std::sin(degree_offset * i);
        const auto y = indicator_radius * std::cos(degree_offset * i);
        const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));

        ImVec4 color;
        color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
        color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
        color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
        color.w = 1.0f;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y), circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
    }
}

bool ImageButtonWithText(long long texturePtr, const char* label, const ImVec2& size)
{
    // 1. 현재 커서 위치(버튼이 그려질 위치)를 저장해둡니다.
    ImVec2 p = ImGui::GetCursorPos();

    // 2. [1층] 이미지를 그립니다.
    // 텍스처가 있으면 그리고, 없으면(0이면) 그냥 빈 공간만 차지하게 둡니다.
    if (texturePtr != 0) {
        ImGui::Image((ImTextureID)texturePtr, size);
    }
    else {
        // 이미지가 아직 로드 안 됐을 때를 대비해 투명 박스 처리
        ImGui::Dummy(size);
    }

    // 3. [2층] 커서를 다시 아까 저장한 위치(이미지 시작점)로 되돌립니다.
    // 이걸 안 하면 버튼이 이미지 아래에 그려집니다.
    ImGui::SetCursorPos(p);

    // 4. [3층] 투명 버튼을 그립니다.
    // 배경색을 투명(Alpha=0)하게 해서 뒤에 있는 이미지가 보이게 합니다.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    // 마우스 올렸을 때 살짝 하얗게 빛나게 (Highlight 효과)
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.2f));

    // 눌렀을 때 좀 더 진하게
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.4f));

    // 실제 버튼 생성 (이름과 크기는 그대로 전달)
    bool clicked = ImGui::Button(label, size);

    // 스타일 복구
    ImGui::PopStyleColor(3);

    return clicked; // 클릭 여부 반환
}

// 헬퍼 함수 구현
std::string CP949ToUTF8(const std::string& strCP949)
{
    if (strCP949.empty()) return "";
    int nwLen = MultiByteToWideChar(949, 0, strCP949.c_str(), -1, NULL, 0);
    wchar_t* pwBuf = new wchar_t[nwLen];
    MultiByteToWideChar(949, 0, strCP949.c_str(), -1, pwBuf, nwLen);
    int nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, 0, NULL, NULL);
    char* pBuf = new char[nLen];
    WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, pBuf, nLen, NULL, NULL);
    std::string strUTF8(pBuf);
    delete[] pwBuf;
    delete[] pBuf;
    return strUTF8;
}
