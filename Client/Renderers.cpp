#include "stdafx.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Mesh.h"
#include "Object.h"

//#include <pix.h>    // PIX 디버깅용

template<typename T>
inline void CRenderer<T>::Initialize(ID3D12Device* dev, UINT instSize)
{
    this->device = dev;
    this->max_capacity = instSize;

    // 상수 버퍼 혹은 구조적 버퍼 리소스 생성 (기존 로직 활용)
    inst_cb = CreateBufferResource(dev, nullptr, nullptr,
        sizeof(T) * instSize,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

    inst_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

template<typename T>
inline void CRenderer<T>::ResizeBuffer(UINT requiredSize)
{
    if (max_capacity >= requiredSize) return;

    if (inst_cb) {
        inst_cb->Unmap(0, nullptr);
        inst_cb.Reset();
    }
    Initialize(device, requiredSize + (requiredSize / 2));
}

template<typename T>
inline void CRenderer<T>::RenderBatches(ID3D12GraphicsCommandList* commandList, UINT rootSlot)
{
    UINT currentOffset = 0;

    UINT total_instances = 0;
    for (auto& [key, instances] : static_batches) total_instances += (UINT)instances.size();
    for (auto& [key, instances] : dynamic_batches) total_instances += (UINT)instances.size();
    if (total_instances == 0) return;

    if (total_instances > max_capacity) {
        ResizeBuffer(total_instances);
    }

    // 1. Static Batches 렌더링 (매 프레임 재생성 안함, 그대로 사용)
    for (auto& [key, instances] : static_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        memcpy(&mapped[currentOffset], instances.data(), sizeof(T) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(T);
        commandList->SetGraphicsRootShaderResourceView(rootSlot, gpuAddr);

        key->Render(commandList, count);
        currentOffset += count;
    }
    static_batches.clear(); // 나중에 제거(오류남)

    // 2. Dynamic Batches 렌더링
    for (auto& [key, instances] : dynamic_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        memcpy(&mapped[currentOffset], instances.data(), sizeof(T) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(T);
        commandList->SetGraphicsRootShaderResourceView(rootSlot, gpuAddr);

        key->Render(commandList, count);
        currentOffset += count;
    }

    dynamic_batches.clear();
}

template<typename T>
inline void CRenderer<T>::AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic)
{
    T data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    if(material)
        data.material = material->GetMaterial()->GetMaterialData();
    else {
        data.material = MaterialData{};
        data.material.albedo = {1, 1, 0, 1};
        data.material.tex_idx = 0;  // texture는 white
    }

    // Mesh별로 배치(Batch) 구성
    if(isStatic)
        static_batches[mesh].push_back(data);
    else
        dynamic_batches[mesh].push_back(data);
}

template<typename T>
inline void CRenderer<T>::AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic)
{
    T data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    data.material = MaterialData{};
    data.material.albedo = color;
    data.material.tex_idx = 0;  // texture는 white

    // Mesh별로 배치(Batch) 구성
    if (isStatic)
        static_batches[mesh].push_back(data);
    else
        dynamic_batches[mesh].push_back(data);
}

// CInstRenderer
void CInstRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    RenderBatches(cmdList, 3);
}

// UIManager가 사각형 하나를 가지고 그림
CUIRenderer::CUIRenderer()
{
    quad_mesh = std::make_shared<CRectangleMesh>(GET_DEVICE, GET_CMD_LIST, 1.0f, 1.0f);
}

void CUIRenderer::AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic)
{
    CRenderer<UIInstCB>::AddInstance(quad_mesh.get(), material, world, isStatic);
}

void CUIRenderer::AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic)
{
    CRenderer<UIInstCB>::AddInstance(quad_mesh.get(), color, world, isStatic);
}

void CUIRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    RenderBatches(cmdList, 1);
}

// CRenderer.cpp 맨 아래 추가
template class CRenderer<InstCB>;
template class CRenderer<UIInstCB>;
//template class CRenderer<BillboardCB>;