#pragma once

class CGBufferTarget
{
public:
	CGBufferTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);

	~CGBufferTarget() = default;

	// 외부(SceneManager 등)에서 최종 디퍼드 셰이더의 SRV 힙 영역에 복사할 때 사용
	void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle);
	
	ID3D12Resource* GetResource() const { return buffer.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return rtv_handle; }
private:
	ComPtr<ID3D12Resource>       buffer;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE                 rtv_handle{};
	DXGI_FORMAT                                  format;
};

class CRenderTarget
{
public:
    CRenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);

    // 렌더링 시작/끝 시 리소스 상태 전이 자동화
    void RenderBegin(ID3D12GraphicsCommandList* cmdList);
    void RenderEnd(ID3D12GraphicsCommandList* cmdList);

    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle);

    ID3D12Resource* GetResource() const { return buffer.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return rtv_handle; }

private:
    ComPtr<ID3D12Resource> buffer;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle{};
    DXGI_FORMAT format;

    // 리소스 상태 전이를 위한 헬퍼
    D3D12_RESOURCE_BARRIER Transition(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
};