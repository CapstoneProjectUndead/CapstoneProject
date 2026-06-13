#include "stdafx.h"
#include <filesystem>
#include "Player.h"
#include "KeyManager.h"
#include "NetworkManager.h"

#include "Timer.h"
#include "Camera.h"
#include "Scene.h"
#include "GameFramework.h"

#include "SceneManager.h"
#include "LobbyScene.h"

#include "ImGuiManager.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "TitleScene.h"
#include "CustomScene.h"
#include "GameScene.h"
#include "UIScene.h"
#include "ItemFactory.h"

extern HWND ghWnd;

CGameFramework::CGameFramework()
	: timer{ CTimer::GetInstance() }
{
	_tcscpy_s(frame_rate_str, _T("Undead ("));

	viewport = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f };
	scissor_rect = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
}

CGameFramework::~CGameFramework()
{

}

bool CGameFramework::OnCreate()
{
	// Direct3D 디바이스, 명령 큐와 명령 리스트 생성, 스왑 체인 등 생성 함수 호출
	CreateD3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvHeaps();
	CreateSwapChain();
	CreateDepthStencilView();

	// 아이템 도감 데이터 읽어오기 (순서 중요! 반드시 씬 초기화 이전에 해야한다.)
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path jsonPath = std::filesystem::path(exePath).parent_path() / "Data/items.json";
	ItemFactory::LoadFromJson(jsonPath.string());
	ItemFactory::LoadModelMap("../Modeling/item/item_model.json");

	// CKeyManager 초기화
	CKeyManager::GetInstance().Init();

	// CSoundManager 초기화
	CSoundManager::GetInstance().Init();

	// ImGuiManager 초기화
	CImGuiManager::GetInstance().Init(ghWnd, GET_DEVICE, GET_CMD_QUEUE, 2, DXGI_FORMAT_R8G8B8A8_UNORM);

	// 렌더링 게임 객체 생성
	BuildObjects();

	// 텍스처 로드
	CResourceManager::GetInstance().LoadAll(d3d_device.Get(), command_queue.Get());

	graphics_memory = std::make_unique<GraphicsMemory>(d3d_device.Get());

	return true;
}

void CGameFramework::OnDestroy()
{
	waitForGpuComplete();

	::CloseHandle(fence_event);

	graphics_memory.reset();

	// 보더리스 전체화면 방식 사용 중이므로 SetFullscreenState 불필요
	if (is_fullscreen) {
		::SetWindowLong(ghWnd, GWL_STYLE, windowed_style);
		::SetWindowPos(ghWnd, HWND_TOP,
			windowed_rect.left, windowed_rect.top,
			windowed_rect.right - windowed_rect.left,
			windowed_rect.bottom - windowed_rect.top,
			SWP_FRAMECHANGED);
	}
#if defined(_DEBUG)
	// 리소스 누수 확인
	IDXGIDebug1* dxgi_debug = nullptr;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&dxgi_debug);
	HRESULT result = dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	dxgi_debug->Release();
#endif
}

// 스왑 체인, 디바이스, 서술자 힙, 명령 큐/ 할당자/ 리스트 생성 함수
void CGameFramework::CreateSwapChain()
{
	RECT rc;
	GetClientRect(ghWnd, &rc);
	client_width = rc.right - rc.left;
	client_height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = swap_chain_buffer_num;
	swapChainDesc.BufferDesc.Width = client_width;
	swapChainDesc.BufferDesc.Height = client_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = ghWnd;
	swapChainDesc.SampleDesc.Count = (msaa4x_enabled) ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = (msaa4x_enabled) ? (msaa4x_quality_level - 1) : 0;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(dxgi_factory->CreateSwapChain(command_queue.Get(), &swapChainDesc, (IDXGISwapChain**)swap_chain.GetAddressOf()));
	swap_chain_buffer_index = swap_chain->GetCurrentBackBufferIndex();
	ThrowIfFailed(dxgi_factory->MakeWindowAssociation(ghWnd, DXGI_MWA_NO_ALT_ENTER));
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE 
	CreateRenderTargetViews();
#endif 
}

void CGameFramework::CreateD3DDevice()
{
	HRESULT result;
	UINT dxgiFactoryFlags{};
#if defined(_DEBUG)
	ID3D12Debug* d3dDebugController{};
	result = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&d3dDebugController);
	if (d3dDebugController) {
		d3dDebugController->EnableDebugLayer();
		d3dDebugController->Release();
	}
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), (void**)&dxgi_factory));

	IDXGIAdapter1* adapter{};
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters1(i, &adapter); ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&d3d_device))) break;
	}
	if (!adapter) {
		dxgi_factory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), (void**)&adapter);
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&d3d_device);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaAQualityLevels;
	msaaAQualityLevels.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	msaaAQualityLevels.SampleCount = 4;
	msaaAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaAQualityLevels.NumQualityLevels = 0;
	d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	msaa4x_quality_level = msaaAQualityLevels.NumQualityLevels;

	msaa4x_enabled = (msaa4x_quality_level > 1) ? true : false;

	result = d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&fence);
	for (int i = 0; i < swap_chain_buffer_num; ++i)
		fence_value[i] = 0;

	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(client_width);
	viewport.Height = static_cast<float>(client_height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissor_rect = { 0, 0, client_width, client_height };

	if (adapter) adapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(d3d_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&command_queue));
	ThrowIfFailed(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&command_allocator));
	ThrowIfFailed(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)command_list.GetAddressOf()));
	ThrowIfFailed(command_list->Close());
}

void CGameFramework::CreateRtvAndDsvHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = swap_chain_buffer_num;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(d3d_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&rtv_descriptor_heap));
	rtv_increment_size = d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(d3d_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&dsv_descriptor_heap));
	dsv_increment_size = d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < swap_chain_buffer_num; ++i) {
		ThrowIfFailed(swap_chain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&render_target_buffers[i]));
		d3d_device->CreateRenderTargetView(render_target_buffers[i].Get(), NULL, rtvCPUDescriptorHandle);
		rtvCPUDescriptorHandle.ptr += rtv_increment_size;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = client_width;
	resourceDesc.Height = client_height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	resourceDesc.SampleDesc.Count = (msaa4x_enabled) ? 4 : 1;
	resourceDesc.SampleDesc.Quality = (msaa4x_enabled) ? (msaa4x_quality_level - 1) : 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(d3d_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&depth_stencil_buffer));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDesctiptorHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	d3d_device->CreateDepthStencilView(
		depth_stencil_buffer.Get(),
		&dsvDesc,
		dsvCPUDesctiptorHandle);
}

void CGameFramework::BuildObjects()
{
	command_list->Reset(command_allocator.Get(), NULL);

	CSceneManager::GetInstance().Init(d3d_device.Get());

	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE] = std::make_unique<CTitleScene>();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY] = std::make_unique<CLobbyScene>();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS] = std::make_unique<CCustomScene>();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME] = std::make_unique<CGameScene>();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::UI] = std::make_unique<CUIScene>();

	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE].get();
	CSceneManager::GetInstance().SetActiveScene(activeScene);

	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE]->Initialize();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY]->Initialize();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS]->Initialize();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME]->Initialize();
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::UI]->Initialize();

	if (activeScene && !IS_CONNECT)
		activeScene->BuildObjects(d3d_device.Get(), command_list.Get());

	command_list->Close();
	ID3D12CommandList* commandLists[] = { command_list.Get() };
	command_queue->ExecuteCommandLists(1, commandLists);

	waitForGpuComplete();

	if (activeScene)
		activeScene->ReleaseUploadBuffers();

	timer.Reset();
}

void CGameFramework::waitForGpuComplete()
{
	const UINT64 fenceValue = ++fence_value[swap_chain_buffer_index];
	ThrowIfFailed(command_queue->Signal(fence.Get(), fenceValue));
	if (fence->GetCompletedValue() < fenceValue) {
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fence_event));
		::WaitForSingleObject(fence_event, INFINITE);
	}
}

void CGameFramework::ChangeSwapChainState()
{
	if (!is_fullscreen) {
		windowed_style = static_cast<DWORD>(::GetWindowLong(ghWnd, GWL_STYLE));
		::GetWindowRect(ghWnd, &windowed_rect);

		MONITORINFO mi = { sizeof(mi) };
		::GetMonitorInfo(::MonitorFromWindow(ghWnd, MONITOR_DEFAULTTONEAREST), &mi);
		int w = mi.rcMonitor.right - mi.rcMonitor.left;
		int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

		::SetWindowLong(ghWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		::SetWindowPos(ghWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, w, h, SWP_FRAMECHANGED | SWP_NOACTIVATE);

		is_fullscreen = true;
		OnResize(w, h);
	}
	else {
		int w = windowed_rect.right - windowed_rect.left;
		int h = windowed_rect.bottom - windowed_rect.top;

		::SetWindowLong(ghWnd, GWL_STYLE, windowed_style);
		::SetWindowPos(ghWnd, HWND_TOP, windowed_rect.left, windowed_rect.top, w, h, SWP_FRAMECHANGED | SWP_NOACTIVATE);

		is_fullscreen = false;
		OnResize(w, h);
	}
}

void CGameFramework::OnResize(UINT width, UINT height)
{
	if (!swap_chain)
		return;

	waitForGpuComplete();

	for (int i = 0; i < swap_chain_buffer_num; ++i) {
		if (render_target_buffers[i]) render_target_buffers[i].Reset();
	}

	if (depth_stencil_buffer) depth_stencil_buffer.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swap_chain->GetDesc(&swapChainDesc);
	ThrowIfFailed(swap_chain->ResizeBuffers(swap_chain_buffer_num, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

	swap_chain_buffer_index = swap_chain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();

	client_width = (int)width;
	client_height = (int)height;

	CreateDepthStencilView();

	viewport = { 0.0f, 0.0f, (float)client_width, (float)client_height, 0.0f, 1.0f };
	scissor_rect = { 0, 0, (long)client_width, (long)client_height };

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)client_width, (float)client_height);

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	if (scene) {
		auto& cam = scene->GetCamera();
		if (cam) {
			cam->SetViewport(0, 0, client_width, client_height);
			cam->SetScissorRect(0, 0, client_width, client_height);
			cam->GenerateProjectionMatrix(0.01f, 500.0f, (float)client_width / (float)client_height, 90.0f);
		}
	}

	CSceneManager::GetInstance().OnResizeBuffers(d3d_device.Get(), client_width, client_height);
	CSceneManager::GetInstance().CreateMainDepthSRV(d3d_device.Get());
}

void CGameFramework::MoveToNextFrame()
{
	swap_chain_buffer_index = swap_chain->GetCurrentBackBufferIndex();

	UINT64 fenceValue = ++fence_value[swap_chain_buffer_index];
	ThrowIfFailed(command_queue->Signal(fence.Get(), fenceValue));
	if (fence->GetCompletedValue() < fenceValue) {
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fence_event));
		WaitForSingleObject(fence_event, INFINITE);
	}
}

void CGameFramework::Update()
{
	timer.Tick(0.0f);

	g_client_total_time += timer.GetTimeElapsed();

	CKeyManager::GetInstance().Tick();
	CSoundManager::GetInstance().Tick();
	CNetworkManager::GetInstance().Tick(0);
	CSceneManager::GetInstance().Update();
	CImGuiManager::GetInstance().Update();
}

void CGameFramework::FrameAdvance()
{
	CommandBegin();

	Update();
	Render();

	CommandEnd();
}

void CGameFramework::Render()
{
	CSceneManager::GetInstance().GetActiveScene()->CollectObjects(command_list.Get());
	RenderBegin();
	CSceneManager::GetInstance().Render(command_list.Get());
	CSceneManager::GetInstance().GetActiveScene()->RenderDeferred(command_list.Get(), depth_stencil_buffer.Get());
	CImGuiManager::GetInstance().Render(command_list.Get());
}

void CGameFramework::RenderBegin()
{
	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor_rect);

	// 현재 프레임 백버퍼 및 메인 깊이 버퍼 핸들 계산
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (swap_chain_buffer_index * rtv_increment_size);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

	// 프레임 시작 상태 연동: 메인 뎁스 버퍼는 기본 상태가 DEPTH_WRITE라고 가정하고 바로 바인딩 및 Clear 시도
	command_list->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	command_list->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	command_list->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void CGameFramework::CommandBegin()
{
	ThrowIfFailed(command_allocator->Reset());
	ThrowIfFailed(command_list->Reset(command_allocator.Get(), NULL));

	// 모니터 메인 백버퍼의 상태 전이를 가장 안전한 파이프라인의 시작 지점으로 이동
	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource = render_target_buffers[swap_chain_buffer_index].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command_list->ResourceBarrier(1, &resourceBarrier);
}

void CGameFramework::CommandEnd()
{
	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource = render_target_buffers[swap_chain_buffer_index].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command_list->ResourceBarrier(1, &resourceBarrier);

	ThrowIfFailed(command_list->Close());

	ID3D12CommandList* commandLists[]{ command_list.Get() };
	command_queue->ExecuteCommandLists(1, commandLists);

	waitForGpuComplete();

	swap_chain->Present(1, 0);

	GraphicsMemory::Get().Commit(command_queue.Get());

	MoveToNextFrame();

	timer.GetFrameRate(frame_rate_str + 8, 37);
	::SetWindowText(ghWnd, frame_rate_str);
}