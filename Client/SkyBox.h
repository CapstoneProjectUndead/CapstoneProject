#pragma once
class CCamera;
class CDescriptorHeapManager;

// 6개의 이미지 로드 및 큐브맵 SRV 생성, 정육면체 Mesh 생성
class CSkyBox
{
public:
    CSkyBox() = default;
    ~CSkyBox() = default;

    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu);
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager);

    void Render(ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager);
private:
    // 큐브맵 DDS 로드 및 SRV 바인딩
    void CreateCubeMapFromFiles(ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager);
private:
    ID3D12Device* d3d_device{};
    ComPtr<ID3D12Resource> cubeMap;
    ComPtr<ID3D12Resource> upload_buffer;

    ComPtr<ID3D12Resource> vertex_buffer;
    ComPtr<ID3D12Resource> index_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
    D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
    UINT index_count = 0;

    UINT srv_index = 0;
};
