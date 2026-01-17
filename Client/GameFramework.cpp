#include "stdafx.h"
#include "GameFramework.h"
#include "Player.h"
#include "KeyManager.h"
#include "NetworkManager.h"

#include "SceneManager.h"
#include "TestScene.h"

extern HWND ghWnd;

CGameFramework::CGameFramework()
{
	_tcscpy_s(frame_rate_str, _T("LapProject ("));

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

	// 렌더링 게임 객체 생성
	BuildObjects();

	// CKeyManager 초기화
	CKeyManager::GetInstance().Init();

	return true;
}
void CGameFramework::OnDestroy()
{
	waitForGpuComplete();

	ReleaseObjects();

	::CloseHandle(fence_event);

	swap_chain->SetFullscreenState(FALSE, nullptr);
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

	// 따라하기05
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
	// 전체 화면 모드에서 바탕화면의 해상도를 후면 버퍼의 크기에 맞게 변경
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(dxgi_factory->CreateSwapChain(command_queue.Get(), &swapChainDesc, (IDXGISwapChain**)swap_chain.GetAddressOf()));
	swap_chain_buffer_index = swap_chain->GetCurrentBackBufferIndex();
	ThrowIfFailed(dxgi_factory->MakeWindowAssociation(ghWnd, DXGI_MWA_NO_ALT_ENTER));	// alt+enter에 응답하지 않게 설정
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE 
	CreateRenderTargetViews();
#endif // 

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
	// factory 생성
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), (void**)&dxgi_factory));

	// 어댑터(그래픽카드)
	IDXGIAdapter1* adapter{};
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters1(i, &adapter); ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;	// 렌더링 드라이버라면?
		if(SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&d3d_device))) break;	// direct12 디바이스 생성
	}
	if (!adapter) {	// 12를 지원하는 디바이스를 생성할 수 없으면 WARP 디바이스 생성
		dxgi_factory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), (void**)&adapter);
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&d3d_device);
	}

	// 디바이스가 지원하는 다중 샘플 품질 수준 확인
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaAQualityLevels;
	msaaAQualityLevels.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	msaaAQualityLevels.SampleCount = 4;	// Msaa4x 다중 샘플링
	msaaAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaAQualityLevels.NumQualityLevels = 0;
	d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	msaa4x_quality_level = msaaAQualityLevels.NumQualityLevels;

	// 품질 수준이 1보다 크면 다중 샘플링 활성화
	msaa4x_enabled = (msaa4x_quality_level > 1) ? true : false;

	// 동기화를 위한 fence 생성
	result = d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&fence);
	for(int i = 0; i < swap_chain_buffer_num; ++i)
		fence_value[i] = 0;

	// 펜스와 동기화를 위한 이벤트 객체 생성. signal 시 이벤트 값을 자동적으로 FALSE가 되도록 생성
	/*
	HANDLE CreateEvent(
	LPSECURITY_ATTRIBUTES lpEventAttributes, // 보안 속성
	BOOL bManualReset,                      // 수동/자동 리셋 여부
	BOOL bInitialState,                     // 초기 상태 (신호/비신호)
	LPCSTR lpName                           // 이벤트 객체 이름
	);
	*/
	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 뷰포트를 윈도우 클라이언트 전체 영역으로 설정
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(client_width);
	viewport.Height = static_cast<float>(client_height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// 씨저 사각형을 클라이언트 영역 전체로 설정
	scissor_rect = { 0, 0, client_width, client_height };

	if (adapter) adapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPU 계산이 오래 걸릴 때 time out 실행 후 다음 명형 실행
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// GPU가 모든 명령 직접 실행

	// 직접 명령 큐 생성
	ThrowIfFailed(d3d_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&command_queue));

	// 할당자 생성
	ThrowIfFailed(d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&command_allocator));

	// 리스트 생성
	ThrowIfFailed(d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)command_list.GetAddressOf()));

	// 리스트가 생성되면 Open상태가 됨
	ThrowIfFailed(command_list->Close());
}


void CGameFramework::CreateRtvAndDsvHeaps()
{
	// 렌더 타겟 서술자 힙(서술자 개수 = 스왑체인 버퍼의 개수) 생성
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = swap_chain_buffer_num;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(d3d_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&rtv_descriptor_heap));

	// 렌더 타겟 서술자 힙 원소의 크기 저장
	rtv_increment_size = d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 깊이-스탠실 서술자 힙(1개) 생성
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(d3d_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&dsv_descriptor_heap));

	// 원소 크기 저장
	dsv_increment_size = d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	// 스왑체인의 각 후면 버퍼에 대한 렌더 타겟 뷰 생성
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
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
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
	// 버퍼 생성
	ThrowIfFailed(d3d_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&depth_stencil_buffer));

	// 뷰 생성
	// 힙 시작 핸들 값
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDesctiptorHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	d3d_device->CreateDepthStencilView(depth_stencil_buffer.Get(), NULL, dsvCPUDesctiptorHandle);
}

// 렌더링할 메쉬와 게임 객체 생성 및 소멸 함수
void CGameFramework::BuildObjects()
{
	command_list->Reset(command_allocator.Get(), NULL);

	// 씬 객체 생성
	CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TEST] = std::make_unique<CTestScene>();

	// 방금 만든 Test Scene을 Active Scene으로 설정
	CScene* testScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TEST].get();
	CSceneManager::GetInstance().SetActiveScene(testScene);

	// Scene에서 생성할 오브젝트들 생성
	if (testScene) 
		testScene->BuildObjects(d3d_device.Get(), command_list.Get());

	// 그래픽 명령 리스트 명령 큐에 추가
	command_list->Close();
	ID3D12CommandList* commandLists[] = { command_list.Get()};
	command_queue->ExecuteCommandLists(1, commandLists);

	//그래픽 명령 리스트들이 모두 실행될 때까지 기다린다.
	waitForGpuComplete();

	//그래픽 리소스들을 생성하는 과정에 생성된 업로드 버퍼들을 소멸시킨다.
	if (testScene) 
		testScene->ReleaseUploadBuffers();

	timer.Reset();
}

void CGameFramework::ReleaseObjects()
{
}

void CGameFramework::waitForGpuComplete()
{
	// CPU 펜스 값 증가
	const UINT64 fenceValue = ++fence_value[swap_chain_buffer_index];
	// GPU가 펜스 값을 설정하는 명령을 명령어 큐에 추가
	ThrowIfFailed(command_queue->Signal(fence.Get(), fenceValue));
	// GPU 펜스 값이 CPU 펜스 값보다 작으면 계속 이벤트를 기다림
	if (fence->GetCompletedValue() < fenceValue) {
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fence_event));
		::WaitForSingleObject(fence_event, INFINITE);
	}
}

void CGameFramework::ChangeSwapChainState()
{
	waitForGpuComplete();

	BOOL fullScreenState{};
	swap_chain->GetFullscreenState(&fullScreenState, NULL);
	swap_chain->SetFullscreenState(!fullScreenState, NULL);	// full -> window, window -> full

	DXGI_MODE_DESC targetParameters;
	targetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	targetParameters.Width = client_width;
	targetParameters.Height = client_height;
	targetParameters.RefreshRate.Numerator = 60;
	targetParameters.RefreshRate.Denominator = 1;
	targetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	targetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain->ResizeTarget(&targetParameters);

	// ResizeBuffers가 성공하기 위해 모든 직/간접 참조를 해제해야 함
	for (int i = 0; i < swap_chain_buffer_num; ++i) {
		if (render_target_buffers[i]) render_target_buffers[i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swap_chain->GetDesc(&swapChainDesc);
	swap_chain->ResizeBuffers(swap_chain_buffer_num, client_width, client_height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);

	swap_chain_buffer_index = swap_chain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
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


void CGameFramework::FrameAdvance()
{
	CommandBegin();

	// Update & 렌더링
	Update();
	Render();
	
	CommandEnd();
}

void CGameFramework::Update()
{
	timer.Tick(0.0f);

	// 키 입력 처리
	CKeyManager::GetInstance().Tick();

	// 네트워크 패킷 처리
	CNetworkManager::GetInstance().Tick(0);

	// Scene 오브젝트 Update
	CSceneManager::GetInstance().Update();
}

void CGameFramework::Render()
{
	CSceneManager::GetInstance().Render(command_list.Get());
}

void CGameFramework::CommandBegin()
{
	// 명령 리셋
	ThrowIfFailed(command_allocator->Reset());
	ThrowIfFailed(command_list->Reset(command_allocator.Get(), NULL));

	// 뷰포트 씨저 사각형 설정
	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor_rect);

	// 현재 렌더 타겟에 대한 프리젠트가 끝나기를 기다림. 프리젠트가 끝나면 렌더 타겟 상태로 바꿈
	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// render target state로 리소스 변경
	resourceBarrier.Transition.pResource = render_target_buffers[swap_chain_buffer_index].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command_list->ResourceBarrier(1, &resourceBarrier);

	// 현재 렌더 타겟에 해당하는 서술자의 CPU 주소(핸들) 값을 계산
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDesciptorHandle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	rtvCPUDesciptorHandle.ptr += (swap_chain_buffer_index * rtv_increment_size);

	// 깊이-스텐실 서술자의 CPU 주소를 계산한다.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

	// 렌더 타겟 뷰와 깊이 스텐실 뷰를 출력 병합 단계(OM)에 연결
	command_list->OMSetRenderTargets(1, &rtvCPUDesciptorHandle, FALSE, &dsvCPUDescriptorHandle);

	// 원하는 색상으로 렌더 타겟 지우기
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	command_list->ClearRenderTargetView(rtvCPUDesciptorHandle, clearColor, 0, NULL);

	// 원하는 값으로 깊이 스텐실 지우기
	command_list->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, NULL);
}

void CGameFramework::CommandEnd()
{
	D3D12_RESOURCE_BARRIER resourceBarrier{};

	// 현재 렌더 타겟에 대한 렌더링이 끝나기를 기다림. GPU가 버퍼를 더 이상 사용하지 않으면 렌더 타겟 -> 프레젠트 상태로 변경
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource =
		render_target_buffers[swap_chain_buffer_index].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	command_list->ResourceBarrier(1, &resourceBarrier);

	ThrowIfFailed(command_list->Close());

	ID3D12CommandList* commandLists[]{ command_list.Get() };
	command_queue->ExecuteCommandLists(1, commandLists);

	waitForGpuComplete();

	// 스왑체인 프리젠트. 현재 렌더 타겟의 내용이 전면 버퍼로 옮겨지고 렌더 타겟 인덱스가 바뀜
	swap_chain->Present(0, 0);

	MoveToNextFrame();

	timer.GetFrameRate(frame_rate_str + 12, 37);
	::SetWindowText(ghWnd, frame_rate_str);
}

void CGameFramework::ProcessMouseMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
	switch (MessageID) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFramework::ProcessKeyboardMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
	switch (MessageID) {
	case WM_KEYUP:
		 switch (wParam) {
		 case VK_ESCAPE:
			 ::PostQuitMessage(0);
			 break;
		 case VK_RETURN:
			 break;
		 case VK_F8:
			 break;
		 case VK_F9:
			 ChangeSwapChainState();
			 break;
		 default:
			 break;
		 }
		 break;
	default:
		 break;
	}
}

void CGameFramework::ProcessWindowMessage(HWND hWnd, UINT MessageID, WPARAM wParam, LPARAM lParam)
{
	switch (MessageID) {
	case WM_SIZE:
	{
		client_width = LOWORD(lParam);
		client_height = HIWORD(lParam);
		break;
	}
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		POINT oldCursorPos;
		::GetCursorPos(&oldCursorPos);
		CKeyManager::GetInstance().SetOldMousePos(oldCursorPos);
	}
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		Vec2 oldPos{ CKeyManager::GetInstance().GetOldMousePos() };
		SetCursorPos(oldPos.x, oldPos.y);
	}
		break;
	case WM_MOUSEMOVE:
		ProcessMouseMessage(hWnd, MessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		ProcessKeyboardMessage(hWnd, MessageID, wParam, lParam);
		break;
	}
}

