#include "stdafx.h"
#include "ShadowMap.h"

CShadowMap::CShadowMap(ID3D12Device* device, UINT width, UINT height)
{
	d3d_device = device;
	this->width = width;
	this->height = height;

    viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    scissor_rect = { 0, 0, (long)width, (long)height };
}

void CShadowMap::RenderBegin(ID3D12GraphicsCommandList* cmdList)
{
	// depth write로 변경
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = shadow_depth_buffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdList->ResourceBarrier(1, &barrier);

	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor_rect);

	// 깊이 버퍼 Clear
	cmdList->ClearDepthStencilView(dsv_cpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// OM 단계 바인딩
	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv_cpu);
}

void CShadowMap::RenderEnd(ID3D12GraphicsCommandList* cmdList)
{
	// gpu가 읽기 전에 read로 변경
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = shadow_depth_buffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdList->ResourceBarrier(1, &barrier);
}

void CShadowMap::CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu, D3D12_CPU_DESCRIPTOR_HANDLE dsvCpu)
{
	// Save references to the descriptors. 
	srv_cpu = srvCpu;
	srv_gpu = srvGpu;
	dsv_cpu = dsvCpu;

	CreateResource();
	//  Create the descriptors
	CreateResourceViews();
}

void CShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((width != newWidth) || (height != newHeight)) {
		width = newWidth;
		height = newHeight;

		CreateResource();

		// New resource, so we need new descriptors to that resource.
		CreateResourceViews();
	}
}

void CShadowMap::CreateResourceViews()
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	d3d_device->CreateShaderResourceView(shadow_depth_buffer.Get(), &srvDesc, srv_cpu);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	d3d_device->CreateDepthStencilView(shadow_depth_buffer.Get(), &dsvDesc, dsv_cpu);
}

void CShadowMap::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // DSV가 D24_S8일 때
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	d3d_device->CreateShaderResourceView(shadow_depth_buffer.Get(), &srvDesc, srvCpu);
}

void CShadowMap::CreateResource()
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	FAILED(d3d_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&shadow_depth_buffer)));
}

// CCubeShadowMap
CCubeShadowMap::CCubeShadowMap(ID3D12Device* device, UINT width, UINT height)
	: CShadowMap(device, width, height)
{
}

void CCubeShadowMap::CreateResourceViews()
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	srvDesc.TextureCubeArray.MostDetailedMip = 0;
	srvDesc.TextureCubeArray.MipLevels = 1;
	srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
	srvDesc.TextureCubeArray.First2DArrayFace = 0;
	// ✨ 핵심 수정: 배열 안에 포함될 총 큐브 맵의 개수 명시
	srvDesc.TextureCubeArray.NumCubes = 1;
	d3d_device->CreateShaderResourceView(shadow_depth_buffer.Get(), &srvDesc, srv_cpu);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.MipSlice = 0;
	dsvDesc.Texture2DArray.FirstArraySlice = 0;
	dsvDesc.Texture2DArray.ArraySize = 1 * 6;
	d3d_device->CreateDepthStencilView(shadow_depth_buffer.Get(), &dsvDesc, dsv_cpu);
}

void CCubeShadowMap::CreateResource()
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 6 * 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	FAILED(d3d_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&shadow_depth_buffer)));
}

void CCubeShadowMap::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	srvDesc.TextureCubeArray.MostDetailedMip = 0;
	srvDesc.TextureCubeArray.MipLevels = 1;
	srvDesc.TextureCubeArray.First2DArrayFace = 0;
	srvDesc.TextureCubeArray.NumCubes = 1;
	d3d_device->CreateShaderResourceView(shadow_depth_buffer.Get(), &srvDesc, srvCpu);
}