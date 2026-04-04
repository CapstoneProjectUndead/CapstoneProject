#include "stdafx.h"
#include "ImGuiManager.h"
#include <WICTextureLoader.h>
#include "d3dx12.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "User.h"

#undef min
#undef max

#define ROOM_MAX_PLAYER 4

HIMC CImGuiManager::default_IMC = nullptr;
bool CImGuiManager::need_reset_focus = false;
ImFont* CImGuiManager::vineritc_font = nullptr;
ImFont* CImGuiManager::elephnt_font = nullptr;
ImFont* CImGuiManager::bold_font = nullptr;
ImFont* CImGuiManager::creepster_font = nullptr;

CImGuiManager::CImGuiManager()
{
}

CImGuiManager::~CImGuiManager()
{
    Release();
}

void CImGuiManager::Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, int numFramesInFlight, DXGI_FORMAT rtvFormat)
{
    // 1. ImGui 컨텍스트 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsLight();

    // 폰트 로드
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgun.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    vineritc_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\VINERITC.TTF", 270.0f);
    elephnt_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ELEPHNT.TTF", 80.0f);
    bold_font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgunbd.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    creepster_font = io.Fonts->AddFontFromFileTTF("../Modeling/font/Creepster-Regular.ttf", 270.0f);

    // Win32 초기화
    ImGui_ImplWin32_Init(hwnd);

    // SRV 힙 생성 (64슬롯: 폰트 1개 + 유저 텍스처 최대 63개)
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = MAX_DESCRIPTORS;
    desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv_desc_heap));

    descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    next_slot = 0;

    // DX12 백엔드 초기화 (새 API: CommandQueue + 슬롯 할당 콜백 사용)
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device           = device;
    initInfo.CommandQueue     = cmdQueue;
    initInfo.NumFramesInFlight = numFramesInFlight;
    initInfo.RTVFormat        = rtvFormat;
    initInfo.SrvDescriptorHeap = srv_desc_heap;

    // ImGui 폰트 텍스처용 슬롯 할당 콜백 (캡처 없는 람다 → 함수 포인터로 변환 가능)
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
        CImGuiManager::GetInstance().AllocDescriptor(cpu, gpu);
    };

    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
        // 단순 선형 할당 방식 — 개별 슬롯 해제 불필요
    };

    ImGui_ImplDX12_Init(&initInfo);
}

void CImGuiManager::AllocDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
{
    *cpu = GetCPUHandle(next_slot);
    *gpu = GetGPUHandle(next_slot);
    next_slot++;
}

D3D12_CPU_DESCRIPTOR_HANDLE CImGuiManager::GetCPUHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srv_desc_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptor_size;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE CImGuiManager::GetGPUHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = srv_desc_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptor_size;
    return handle;
}

void CImGuiManager::LoadTexture(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                                 const std::string& name, const std::wstring& path)
{
    if (textures.count(name))
        return; // 중복 로드 방지

    // WIC는 COM 기반 — 초기화가 안 된 상태면 첫 로드가 E_NOINTERFACE로 실패함
    HRESULT res = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    assert(SUCCEEDED(res) && "CoInitializeEx Failed");

    // 1. WIC로 PNG 디코딩 + D3D12 리소스 헤더 생성 (CPU 단계)
    ComPtr<ID3D12Resource> resource;
    std::unique_ptr<uint8_t[]> decodedData;
    D3D12_SUBRESOURCE_DATA subresourceData;

    HRESULT hr = DirectX::LoadWICTextureFromFileEx(
        device, path.c_str(),
        0,
        D3D12_RESOURCE_FLAG_NONE,
        DirectX::WIC_LOADER_FORCE_RGBA32,
        resource.GetAddressOf(),
        decodedData, subresourceData);

    if (FAILED(hr))
        return;

    // 2. 업로드 버퍼 생성
    const UINT64 bufferSize = GetRequiredIntermediateSize(resource.Get(), 0, 1);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&uploadBuffer));

    if (FAILED(hr))
        return;

    // 3. 임시 커맨드 리스트로 GPU에 업로드
    ComPtr<ID3D12CommandAllocator>    tempAlloc;
    ComPtr<ID3D12GraphicsCommandList> tempList;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tempAlloc));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tempAlloc.Get(), nullptr, IID_PPV_ARGS(&tempList));

    UpdateSubresources(tempList.Get(), resource.Get(), uploadBuffer.Get(), 0, 0, 1, &subresourceData);

    // 업로드 완료 후 픽셀 셰이더 읽기 상태로 전환
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    tempList->ResourceBarrier(1, &barrier);
    tempList->Close();

    ID3D12CommandList* cmdLists[] = { tempList.Get() };
    cmdQueue->ExecuteCommandLists(1, cmdLists);

    // GPU 업로드 완료 대기
    ComPtr<ID3D12Fence> uploadFence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));
    HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    cmdQueue->Signal(uploadFence.Get(), 1);
    uploadFence->SetEventOnCompletion(1, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
    CloseHandle(fenceEvent);

    // 4. SRV 생성 및 힙 슬롯에 등록
    UINT slot = next_slot++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format                  = resource->GetDesc().Format;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels     = resource->GetDesc().MipLevels;
    device->CreateShaderResourceView(resource.Get(), &srvDesc, GetCPUHandle(slot));

    UITexture uiTex;
    uiTex.resource = resource;
    uiTex.tex_id   = (ImTextureID)GetGPUHandle(slot).ptr;
    textures[name] = std::move(uiTex);
}

ImTextureID CImGuiManager::GetTexture(const std::string& name) const
{
    auto it = textures.find(name);
    if (it == textures.end())
        return 0;

    return it->second.tex_id;
}

void CImGuiManager::Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 해상도가 0일 때는 ImGui Update를 하지 않는다.
    if (ImGui::GetIO().DisplaySize.x <= 0.0f || ImGui::GetIO().DisplaySize.y <= 0.0f)
        return;

    CScene* activeScene = CSceneManager::GetInstance().GetActiveScene();
    if (activeScene) {
        activeScene->DrawUI_Final();
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
    if (default_IMC != nullptr) 
        return;

    default_IMC = ImmAssociateContext(hwnd, NULL);
}

void CImGuiManager::EnableIME(HWND hwnd)
{
    if (default_IMC == nullptr) 
        return;

    ImmAssociateContext(hwnd, default_IMC);
    default_IMC = nullptr;
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

std::string UTF8ToCP949(const std::string& utf8Str) 
{
    if (utf8Str.empty()) 
        return "";

    // 1. UTF-8 -> Unicode (WideChar) 변환
    // 필요한 버퍼 크기 계산
    int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), NULL, 0);
    std::wstring wstr(nLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), &wstr[0], nLen);

    // 2. Unicode -> CP949 (ANSI) 변환
    // 949는 한국어 코드 페이지입니다.
    int nLen2 = WideCharToMultiByte(949, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strCP949(nLen2, 0);
    WideCharToMultiByte(949, 0, wstr.c_str(), (int)wstr.size(), &strCP949[0], nLen2, NULL, NULL);

    return strCP949;
}
