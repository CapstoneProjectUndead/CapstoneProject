#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <vector>
#include <wrl.h>

// 1. DX12 핵심 헤더들
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// 2. 도우미 헤더 (파일이 프로젝트 폴더에 있어야 해!)
#include "d3dx12.h" 

// 3. 우리 맵 생성기
#include "MapGenerator.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;

// =========================================================
// 🎥 전역 변수 (엔진 핵심 부품)
// =========================================================
HWND g_hwnd = NULL;
ComPtr<ID3D12Device> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<IDXGISwapChain3> g_swapChain;
ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
ComPtr<ID3D12CommandAllocator> g_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<ID3D12Resource> g_renderTargets[2];
ComPtr<ID3D12Fence> g_fence;
HANDLE g_fenceEvent;
UINT64 g_fenceValue = 0;
UINT g_frameIndex = 0;

vector<InstanceData> g_myMap;
ComPtr<ID3D12RootSignature> g_rootSignature;
ComPtr<ID3D12PipelineState> g_pipelineState;

// 🌟 카메라와 데이터 버퍼들
XMMATRIX g_viewProjMatrix;

ComPtr<ID3D12Resource> g_vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;

ComPtr<ID3D12Resource> g_indexBuffer;
D3D12_INDEX_BUFFER_VIEW g_indexBufferView;

ComPtr<ID3D12Resource> g_instanceBuffer;
D3D12_VERTEX_BUFFER_VIEW g_instanceView;

// 🧊 기본 1x1x1 큐브 모델 (버텍스와 인덱스)
struct Vertex { XMFLOAT3 pos; };

Vertex cubeVertices[] = {
    { XMFLOAT3(-0.5f, -0.5f, -0.5f) }, { XMFLOAT3(-0.5f,  0.5f, -0.5f) },
    { XMFLOAT3(0.5f,  0.5f, -0.5f) }, { XMFLOAT3(0.5f, -0.5f, -0.5f) },
    { XMFLOAT3(-0.5f, -0.5f,  0.5f) }, { XMFLOAT3(-0.5f,  0.5f,  0.5f) },
    { XMFLOAT3(0.5f,  0.5f,  0.5f) }, { XMFLOAT3(0.5f, -0.5f,  0.5f) }
};

uint16_t cubeIndices[] = {
    0, 1, 2, 0, 2, 3, // 앞면
    4, 6, 5, 4, 7, 6, // 뒷면
    4, 5, 1, 4, 1, 0, // 왼쪽면
    3, 2, 6, 3, 6, 7, // 오른쪽면
    1, 5, 6, 1, 6, 2, // 윗면
    4, 0, 3, 4, 3, 7  // 아랫면
};

// =========================================================
// ⚙️ 윈도우 창 메시지 처리 (이게 빠져 있었어!)
// =========================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// =========================================================
// ⚙️ 핵심 함수들
// =========================================================

void WaitForGpu() {
    g_fenceValue++;
    g_commandQueue->Signal(g_fence.Get(), g_fenceValue);

    // GPU가 아직 목표 지점(g_fenceValue)에 도달하지 못했다면 기다린다!
    if (g_fence->GetCompletedValue() < g_fenceValue) {

        // 🔥 [바로 이 줄이 빠져있었어!!] GPU야, 작업 끝나면 나한테 카톡(Event) 좀 보내줘!
        g_fence->SetEventOnCompletion(g_fenceValue, g_fenceEvent);

        // 카톡이 올 때까지 무한 대기
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }
}

void InitDX12(HWND hwnd) {
    ID3D12Device* pRawDevice = nullptr;
    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pRawDevice));
    g_device = pRawDevice;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(g_commandQueue.GetAddressOf()));

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.BufferCount = 2;
    sd.Width = 1280; sd.Height = 960; // 👈 여기도 720을 960으로!
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.SampleDesc.Count = 1;

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
    ComPtr<IDXGISwapChain1> swapChain;
    factory->CreateSwapChainForHwnd(g_commandQueue.Get(), hwnd, &sd, nullptr, nullptr, swapChain.GetAddressOf());
    swapChain.As(&g_swapChain);
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(g_rtvHeap.GetAddressOf()));

    UINT rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT n = 0; n < 2; n++) {
        g_swapChain->GetBuffer(n, IID_PPV_ARGS(g_renderTargets[n].GetAddressOf()));
        g_device->CreateRenderTargetView(g_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(g_commandAllocator.GetAddressOf()));
    g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), nullptr, IID_PPV_ARGS(g_commandList.GetAddressOf()));
    g_commandList->Close();

    g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(g_fence.GetAddressOf()));
    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void InitPipeline() {
    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsConstants(16, 0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    D3DCompileFromFile(L"MapShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr);
    D3DCompileFromFile(L"MapShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr);

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INSTANCE_POS",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_SCALE", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 12, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 24, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = g_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;


    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState));
}

void CreateBuffers() {
    // 1. 카메라 세팅 (드론 조종하기!)
    // 🔥 [수정 1] 드론 위치: 높이(Y)를 살짝 낮추고, 앞으로(Z) 당겼어!
    XMVECTOR cameraPos = XMVectorSet(50.0f, 200.0f, -0.0f, 0.0f);

    // 🔥 [수정 2] 카메라 렌즈 방향: 맵의 살짝 뒤쪽(Z를 70으로)을 쳐다보게 해서 맵을 화면 아래로 내림!
    XMVECTOR cameraTarget = XMVectorSet(50.0f, 0.0f, 90.0f, 0.0f);

    XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(cameraPos, cameraTarget, cameraUp);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
    g_viewProjMatrix = XMMatrixMultiply(view, proj);

    // ... (이 아래 uploadHeap 등등은 그대로 두면 돼!)

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    UINT vbSize = sizeof(cubeVertices);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    g_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_vertexBuffer));
    void* pVertexDataBegin;
    g_vertexBuffer->Map(0, nullptr, &pVertexDataBegin);
    memcpy(pVertexDataBegin, cubeVertices, vbSize);
    g_vertexBuffer->Unmap(0, nullptr);
    g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
    g_vertexBufferView.StrideInBytes = sizeof(Vertex);
    g_vertexBufferView.SizeInBytes = vbSize;

    UINT ibSize = sizeof(cubeIndices);
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
    g_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_indexBuffer));
    void* pIndexDataBegin;
    g_indexBuffer->Map(0, nullptr, &pIndexDataBegin);
    memcpy(pIndexDataBegin, cubeIndices, ibSize);
    g_indexBuffer->Unmap(0, nullptr);
    g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
    g_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    g_indexBufferView.SizeInBytes = ibSize;

    // 🌟 [핵심] 여기서 비어있지 않은 g_myMap 데이터를 사용해야 해!
    UINT instSize = (UINT)(g_myMap.size() * sizeof(InstanceData));
    CD3DX12_RESOURCE_DESC instDesc = CD3DX12_RESOURCE_DESC::Buffer(instSize);
    g_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &instDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_instanceBuffer));
    void* pInstDataBegin;
    g_instanceBuffer->Map(0, nullptr, &pInstDataBegin);
    memcpy(pInstDataBegin, g_myMap.data(), instSize);
    g_instanceBuffer->Unmap(0, nullptr);
    g_instanceView.BufferLocation = g_instanceBuffer->GetGPUVirtualAddress();
    g_instanceView.StrideInBytes = sizeof(InstanceData);
    g_instanceView.SizeInBytes = instSize;
}
void Render() {
    g_commandAllocator->Reset();
    g_commandList->Reset(g_commandAllocator.Get(), g_pipelineState.Get());

    CD3DX12_RESOURCE_BARRIER barrierEnter[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(g_renderTargets[g_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };
    g_commandList->ResourceBarrier(1, barrierEnter);

    // 1. 도화지 이름표 가져오기
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_frameIndex, g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    // ========================================================
    // 🔥 [내가 빼먹은 범인 검거!] "이 도화지에 그려라!" 하고 세팅해주기
    // ========================================================
    g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 2. 도화지를 남색으로 칠하기
    const float clearColor[] = { 0.05f, 0.05f, 0.15f, 1.0f };
    g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // 3. 화면 뷰포트 설정
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, 1280, 720 };
    g_commandList->RSSetViewports(1, &viewport);
    g_commandList->RSSetScissorRects(1, &scissorRect);

    // 4. 그리기 세팅 및 발사!
    g_commandList->SetPipelineState(g_pipelineState.Get());
    g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());
    g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    XMMATRIX transposedMatrix = XMMatrixTranspose(g_viewProjMatrix);
    g_commandList->SetGraphicsRoot32BitConstants(0, 16, &transposedMatrix, 0);

    g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
    g_commandList->IASetVertexBuffers(1, 1, &g_instanceView);
    g_commandList->IASetIndexBuffer(&g_indexBufferView);

    g_commandList->DrawIndexedInstanced(36, (UINT)g_myMap.size(), 0, 0, 0);

    CD3DX12_RESOURCE_BARRIER barrierExit[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(g_renderTargets[g_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)
    };
    g_commandList->ResourceBarrier(1, barrierExit);

    g_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
    g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    g_swapChain->Present(1, 0);
    WaitForGpu();
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
}

// =========================================================
// 🏁 메인 실행부
// =========================================================
int main() {
    AllocConsole();
    freopen("CONOUT$", "wt", stdout);
    cout << "🚀 DX12 엔진 구동 시작..." << endl;

    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DX12WindowClass";
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(0, L"DX12WindowClass", L"3D Map Project", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 960, NULL, NULL, hInst, NULL); // 👈 720을 960으로!
    ShowWindow(g_hwnd, SW_SHOW);

    InitDX12(g_hwnd);
    InitPipeline();

    // 🌟 [순서 수정!] 맵 데이터를 먼저 만들어야 해!
    cout << "🗺️ 맵 데이터 생성 중..." << endl;
    g_myMap = MapGenerator::Generate3DMap();
    cout << "✅ 생성 완료! 상자 수: " << g_myMap.size() << endl;

    // 🌟 그리고 나서 그 데이터를 그래픽 카드로 복사!
    CreateBuffers();

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Render();
        }
    }

    return 0;
}