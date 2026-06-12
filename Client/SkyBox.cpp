#include "stdafx.h"
#include "SkyBox.h"
#include "Camera.h"
#include "Shader.h"
#include "DDSTextureLoader.h"

void CSkyBox::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager)
{
    srv_index = 0;
    d3d_device = device;

    CreateCubeMapFromFiles(cmdList, heapManager);

    float w = 1.0f; float h = 1.0f; float d = 1.0f;
    struct SkyboxVertex { XMFLOAT3 Pos; };
    SkyboxVertex vertices[] = {
        { XMFLOAT3(-w, +h, -d) }, { XMFLOAT3(+w, +h, -d) },
        { XMFLOAT3(+w, -h, -d) }, { XMFLOAT3(-w, -h, -d) },
        { XMFLOAT3(-w, +h, +d) }, { XMFLOAT3(+w, +h, +d) },
        { XMFLOAT3(+w, -h, +d) }, { XMFLOAT3(-w, -h, +d) }
    };

    UINT indices[] = {
        0, 1, 2, 0, 2, 3, // Front
        5, 4, 7, 5, 7, 6, // Back
        4, 0, 3, 4, 3, 7, // Left
        1, 5, 6, 1, 6, 2, // Right
        4, 5, 1, 4, 1, 0, // Top
        3, 2, 6, 3, 6, 7  // Bottom
    };
    index_count = _countof(indices);

    // Vertex Buffer 생성
    size_t vBufferSize = sizeof(vertices);
    D3D12_HEAP_PROPERTIES vHeapProps{};
    vHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vResDesc{};
    vResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vResDesc.Width = vBufferSize;
    vResDesc.Height = 1;
    vResDesc.DepthOrArraySize = 1;
    vResDesc.MipLevels = 1;
    vResDesc.SampleDesc.Count = 1;
    vResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreateCommittedResource(&vHeapProps, D3D12_HEAP_FLAG_NONE, &vResDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertex_buffer)));

    void* pVertexData = nullptr;
    vertex_buffer->Map(0, nullptr, &pVertexData);
    memcpy(pVertexData, vertices, vBufferSize);
    vertex_buffer->Unmap(0, nullptr);

    vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
    vertex_buffer_view.StrideInBytes = sizeof(SkyboxVertex);
    vertex_buffer_view.SizeInBytes = (UINT)vBufferSize;

    // Index Buffer 생성
    size_t iBufferSize = sizeof(indices);
    D3D12_HEAP_PROPERTIES iHeapProps{};
    iHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC iResDesc{};
    iResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    iResDesc.Width = iBufferSize;
    iResDesc.Height = 1;
    iResDesc.DepthOrArraySize = 1;
    iResDesc.MipLevels = 1;
    iResDesc.SampleDesc.Count = 1;
    iResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreateCommittedResource(&iHeapProps, D3D12_HEAP_FLAG_NONE, &iResDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&index_buffer)));

    void* pIndexData = nullptr;
    index_buffer->Map(0, nullptr, &pIndexData);
    memcpy(pIndexData, indices, iBufferSize);
    index_buffer->Unmap(0, nullptr);

    index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
    index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
    index_buffer_view.SizeInBytes = (UINT)iBufferSize;
}

void CSkyBox::CreateCubeMapFromFiles(ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager)
{
    std::wstring cubeMapFile = L"../Modeling/tex/skybox_cubemap.dds";

    ThrowIfFailed(CreateDDSTextureFromFile12(
        d3d_device,
        cmdList,
        cubeMapFile.c_str(),
        cubeMap,
        upload_buffer
    ));

    D3D12_RESOURCE_DESC texDesc = cubeMap->GetDesc();

    // 셰이더가 파싱할 완전한 입체 구조(TEXTURECUBE) 뷰 디스크립션 구성
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = texDesc.MipLevels;

    // SRV 생성
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapManager->GetSRVHeap().GetCPUHandle(srv_index);
    d3d_device->CreateShaderResourceView(cubeMap.Get(), &srvDesc, cpuHandle);
}

void CSkyBox::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu)
{
    D3D12_RESOURCE_DESC texDesc = cubeMap->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = texDesc.MipLevels;

    d3d_device->CreateShaderResourceView(cubeMap.Get(), &srvDesc, srvCpu);
}

void CSkyBox::Render(ID3D12GraphicsCommandList* cmdList, CDescriptorHeapManager* heapManager)
{
    if (!heapManager || !cubeMap) return;

    ID3D12DescriptorHeap* ppHeaps[] = { heapManager->GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    D3D12_GPU_DESCRIPTOR_HANDLE skyboxHandle = heapManager->GetSRVHeap().GetGPUHandle(srv_index);
    cmdList->SetGraphicsRootDescriptorTable(2, skyboxHandle);

    cmdList->IASetVertexBuffers(0, 1, &vertex_buffer_view);
    cmdList->IASetIndexBuffer(&index_buffer_view);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}