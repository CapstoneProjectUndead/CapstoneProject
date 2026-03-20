#include "stdafx.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Mesh.h"
#include "Object.h"

#include <pix.h>    // PIX 디버깅용


void CInstRenderer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT instSize)
{
    this->device = device;
    max_capacity = instSize;
    inst_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<InstCB>() * instSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
    inst_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

void CInstRenderer::ResizeBuffer(UINT requiredSize)
{
    // 이미 충분한 크기라면 무시
    if (max_capacity >= requiredSize) return;

    // 모자라다면 필요한 크기보다 좀 더 넉넉하게(여유분 50% 추가) 재할당합니다.
    max_capacity = requiredSize + (requiredSize / 2);

    // 기존 버퍼가 있다면 매핑 해제 후 메모리 반환
    if (inst_cb) {
        inst_cb->Unmap(0, nullptr);
        inst_cb.Reset();
    }

    // 새로운 크기로 다시 생성
    Initialize(device, nullptr, requiredSize);
}

void CInstRenderer::AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world)
{
    InstCB data;
    // world matrix
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    // material
    data.material = material->GetMaterial()->GetMaterialData();

    if (material->owner->GetObjectType() != OBJECT_TYPE::STATIC_OBJECT)
        dynamic_batches[{mesh, material}].push_back(data);
    else
        static_batches[{mesh, material}].push_back(data);
}

void CInstRenderer::Render(ID3D12GraphicsCommandList* commandList)
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

        memcpy(&mapped[currentOffset], instances.data(), sizeof(InstCB) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(InstCB);
        commandList->SetGraphicsRootShaderResourceView(3, gpuAddr);

        key.mesh->Render(commandList, count);
        currentOffset += count;
    }
    static_batches.clear();

    // 2. Dynamic Batches 렌더링
    for (auto& [key, instances] : dynamic_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        memcpy(&mapped[currentOffset], instances.data(), sizeof(InstCB) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(InstCB);
        commandList->SetGraphicsRootShaderResourceView(3, gpuAddr);

        key.mesh->Render(commandList, count);
        currentOffset += count;
    }

    dynamic_batches.clear();
}

// UIManager가 사각형 하나를 가지고 그림
CUIRenderer::CUIRenderer()
{
    quad_mesh = std::make_shared<CRectangleMesh>(GET_DEVICE, GET_CMD_LIST, 1.0f, 1.0f);
}

void CUIRenderer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT instSize)
{
    max_capacity = instSize;
    inst_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<UIInstCB>() * instSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
    inst_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

void CUIRenderer::ResizeBuffer(UINT requiredSize)
{
    if (max_capacity >= requiredSize) return;

    max_capacity = requiredSize + (requiredSize / 2);

    // 기존 버퍼가 있다면 매핑 해제 후 메모리 반환
    if (inst_cb) {
        inst_cb->Unmap(0, nullptr);
        inst_cb.Reset();
    }

    Initialize(GET_DEVICE, nullptr, requiredSize);
}

void CUIRenderer::AddInstance(XMFLOAT4 color, const XMFLOAT4X4& world)
{
    UIInstCB data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    data.color = color;

    // Mesh별로 배치(Batch) 구성
    ui_batches[quad_mesh].push_back(data);
}

void CUIRenderer::Render(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UI_Render_Section");
    UINT currentOffset = 0;

    // 1. Orthographic Projection Matrix 설정
    // 화면 좌측 상단(0,0), 우측 하단(width, height) 기준 행렬을 RootConstant 등으로 전달해야 함
    for (auto& [mesh, instances] : ui_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        // 버퍼 크기 체크 및 복사 (기존 CInstRenderer 로직과 동일)
        memcpy(&mapped[currentOffset], instances.data(), sizeof(UIInstCB) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(UIInstCB);

        // UI용 Shader Resource View(SRV) 바인딩 (인스턴싱 데이터)
        commandList->SetGraphicsRootShaderResourceView(1, gpuAddr);

        mesh->Render(commandList, count);
        currentOffset += count;
    }
    ui_batches.clear();
    PIXEndEvent(commandList);
}