#pragma once
class CShadowMap
{
public:
    CShadowMap(ID3D12Device* device, UINT width, UINT height);
    ~CShadowMap() = default;

    // (Depth Write <-> Shader Resource)
    void RenderBegin(ID3D12GraphicsCommandList* cmdList);
    void RenderEnd(ID3D12GraphicsCommandList* cmdList);

    // Getters
    ID3D12Resource* GetResource() const { return shadow_depth_buffer.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCpuHandle() const { return srv_cpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srv_gpu; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle() const { return dsv_cpu; }
    D3D12_VIEWPORT GetViewport() const { return viewport; }
    D3D12_RECT GetScissorRect() const { return scissor_rect; }

    UINT GetWidth()const { return width; }
    UINT GetHeight()const { return height; }

    void CreateDescriptors( D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu, D3D12_CPU_DESCRIPTOR_HANDLE dsvCpu);
    // create srv of shadow_depth_buffer in other shader
    virtual void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu);

    void OnResize(UINT newWidth, UINT newHeight);
protected:
    virtual void CreateResourceViews();
    virtual void CreateResource();
protected:
    ComPtr<ID3D12Resource> shadow_depth_buffer;
    ID3D12Device* d3d_device{};

    D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_cpu;

    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor_rect;
    UINT width{};
    UINT height{};
    DXGI_FORMAT format{ DXGI_FORMAT_R24G8_TYPELESS };
};

class CCubeShadowMap : public CShadowMap {
public:
    CCubeShadowMap(ID3D12Device* device, UINT width, UINT height);
    ~CCubeShadowMap() = default;

    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu) override;
private:
    void CreateResourceViews() override;
    void CreateResource() override;
};