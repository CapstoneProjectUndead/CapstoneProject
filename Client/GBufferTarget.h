#pragma once

class CGBufferTarget
{
public:
    CGBufferTarget(ID3D12Device* device, int width, int height, DXGI_FORMAT format);
    ~CGBufferTarget() = default;

    // 크기 변경을 위한 메서드 추가
    void Resize(ID3D12Device* device, int width, int height);
    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle);

    ID3D12Resource* GetResource() const { return buffer.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return rtv_handle; }

private:
    ComPtr<ID3D12Resource>       buffer;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE  rtv_handle{};
    DXGI_FORMAT                  format;
};

class CRenderTarget
{
public:
    CRenderTarget(ID3D12Device* device, int width, int height, DXGI_FORMAT format);
    ~CRenderTarget() = default;

    // 크기 변경을 위한 메서드 추가
    void Resize(ID3D12Device* device, int width, int height);

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

    D3D12_RESOURCE_BARRIER Transition(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
};