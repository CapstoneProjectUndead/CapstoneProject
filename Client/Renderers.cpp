#include "stdafx.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Mesh.h"
#include "Object.h"
#include "SceneManager.h"
#include "Camera.h"
#include "ResourceUploadBatch.h"
#include "AnimationManager.h"
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

        key.mesh->Render(commandList, key.submesh_index, count);
        currentOffset += count;
    }

    // 2. Dynamic Batches 렌더링
    for (auto& [key, instances] : dynamic_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        memcpy(&mapped[currentOffset], instances.data(), sizeof(T) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(T);
        commandList->SetGraphicsRootShaderResourceView(rootSlot, gpuAddr);

        key.mesh->Render(commandList, key.submesh_index, count);
        currentOffset += count;
    }
}

template<typename T>
void CRenderer<T>::RenderShadowBatches(ID3D12GraphicsCommandList* commandList, UINT rootSlot)
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

        key.mesh->RenderShadow(commandList, key.submesh_index, count);
        currentOffset += count;
    }

    // 2. Dynamic Batches 렌더링
    for (auto& [key, instances] : dynamic_batches) {
        UINT count = (UINT)instances.size();
        if (count == 0) continue;

        memcpy(&mapped[currentOffset], instances.data(), sizeof(T) * count);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
        gpuAddr += currentOffset * sizeof(T);
        commandList->SetGraphicsRootShaderResourceView(rootSlot, gpuAddr);

        key.mesh->RenderShadow(commandList, key.submesh_index, count);
        currentOffset += count;
    }
}

template<typename T>
inline void CRenderer<T>::AddInstance(std::shared_ptr<CMesh> mesh, std::shared_ptr<CMaterialComponent> material, const XMFLOAT4X4& world, UINT submeshIndex, bool isStatic)
{
    T data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    if (material && material->GetMaterial() != nullptr) {
        data.material = material->GetMaterial()->GetMaterialData();
    }
    else {
        data.material = MaterialData{};
        data.material.albedo = {1, 1, 0, 1};
        data.material.tex_idx = 0;  // texture는 white
    }

    RenderKey key{ mesh, submeshIndex };

    // Mesh별로 배치(Batch) 구성
    if(isStatic)
        static_batches[key].push_back(data);
    else
        dynamic_batches[key].push_back(data);
}

template<typename T>
inline void CRenderer<T>::AddInstance(std::shared_ptr<CMesh> mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic)
{
    T data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    data.material = MaterialData{};
    data.material.albedo = color;
    data.material.tex_idx = 0;  // texture는 white

    RenderKey key{ mesh, 0 };

    // Mesh별로 배치(Batch) 구성
    if (isStatic)
        static_batches[key].push_back(data);
    else
        dynamic_batches[key].push_back(data);
}

template<typename T>
inline void CRenderer<T>::AddInstance(std::shared_ptr<CMesh> mesh, const T& customData, UINT submeshIndex, bool isStatic)
{
    T data = customData;

    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&data.world_matrix));
    XMStoreFloat4x4(&data.world_matrix, worldT);

    RenderKey key{ mesh, submeshIndex };

    if (isStatic)
        static_batches[key].push_back(data);
    else
        dynamic_batches[key].push_back(data);
}

// CInstRenderer
void CInstRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    RenderBatches(cmdList, 3);
}

void CAniRenderer::AddInstance(std::shared_ptr<CMesh> mesh, std::shared_ptr<CMaterialComponent> material, const XMFLOAT4X4& world, UINT submeshIndex, bool isStatic)
{
    AniCB data;
    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);
    if (material && material->GetMaterial() != nullptr) {
        data.material = material->GetMaterial()->GetMaterialData();
    }
    else {
        data.material = MaterialData{};
        data.material.albedo = { 1, 1, 0, 1 };
        data.material.tex_idx = 0;  // texture는 white
    }
    data.ani_data = AnimationData{};

    RenderKey key{ mesh, submeshIndex };

    // Mesh별로 배치(Batch) 구성
    if (isStatic)
        static_batches[key].push_back(data);
    else
        dynamic_batches[key].push_back(data);
}

// CAniRenderer
void CAniRenderer::AddInstance(std::shared_ptr<CMesh> mesh, std::shared_ptr<CMaterialComponent> material, const XMFLOAT4X4& world, UINT submeshIndex, AnimationData aniData)
{
    AniCB data;
    if (material && material->GetMaterial() != nullptr) {
        data.material = material->GetMaterial()->GetMaterialData();
    }
    else {
        data.material = MaterialData{};
        data.material.albedo = { 1, 1, 0, 1 };
        data.material.tex_idx = 0;  // texture는 white
    }
    data.ani_data = aniData;

    XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
    XMStoreFloat4x4(&data.world_matrix, worldT);

    RenderKey key{ mesh, submeshIndex };

    dynamic_batches[key].push_back(data);
}

void CAniRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    auto animBuffer = CAnimationManager::GetInstance().GetTextureResource();
    auto maskBuffer = CAnimationManager::GetInstance().GetMaskBuffer();

    cmdList->SetGraphicsRootShaderResourceView(4, animBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(5, maskBuffer->GetGPUVirtualAddress());
    RenderBatches(cmdList, 3);
}

void CShadowRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    auto animBuffer = CAnimationManager::GetInstance().GetTextureResource();
    auto maskBuffer = CAnimationManager::GetInstance().GetMaskBuffer();

    cmdList->SetGraphicsRootShaderResourceView(4, animBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(5, maskBuffer->GetGPUVirtualAddress());
    RenderShadowBatches(cmdList, 3);
}

// UIManager가 사각형 하나를 가지고 그림
CUIRenderer::CUIRenderer()
{
    quad_mesh = std::make_shared<CRectangleMesh>(GET_DEVICE, GET_CMD_LIST, 1.0f, 1.0f);
}

void CUIRenderer::AddInstance(std::shared_ptr<CMesh> mesh, std::shared_ptr<CMaterialComponent> material, const XMFLOAT4X4& world, UINT submeshIndex, bool isStatic)
{
    CRenderer<UIInstCB>::AddInstance(quad_mesh, material, world, submeshIndex, isStatic);
}

void CUIRenderer::AddInstance(std::shared_ptr<CMesh> mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic)
{
    CRenderer<UIInstCB>::AddInstance(quad_mesh, color, world, isStatic);
}

void CUIRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    RenderBatches(cmdList, 1);
}

CBillboardRenderer::CBillboardRenderer()
{
    b_mesh = std::make_shared<CBillboardMesh>(GET_DEVICE, GET_CMD_LIST, 1.0f, 1.0f);
}

void CBillboardRenderer::AddInstance(std::shared_ptr<CMesh> mesh, std::shared_ptr<CMaterialComponent> material, const XMFLOAT4X4& world, UINT submeshIndex, bool isStatic)
{
    CRenderer<BillboardInstCB>::AddInstance(b_mesh, material, world, submeshIndex, isStatic);
}

void CBillboardRenderer::AddInstance(std::shared_ptr<CMesh> mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic)
{
    CRenderer<BillboardInstCB>::AddInstance(b_mesh, color, world, isStatic);
}

void CBillboardRenderer::AddParticleInstance(std::shared_ptr<CMesh> mesh, const BillboardInstCB& particleData, bool isStatic)
{
    CRenderer<BillboardInstCB>::AddInstance(b_mesh, particleData, 0, isStatic);
}

void CBillboardRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    RenderBatches(cmdList, 1);
}

// CTextRenderer
void CTextRenderer::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ResourceUploadBatch uploadBatch(device);
    uploadBatch.Begin();

    // SpriteBatch 생성
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // RenderTargetState 생성
    RenderTargetState rtState(backBufferFormat, depthBufferFormat);

    SpriteBatchPipelineStateDescription psoDesc(rtState);

    sprite_batch = std::make_unique<SpriteBatch>(device, uploadBatch, psoDesc);

    // SpriteFont 생성
    default_font = std::make_unique<SpriteFont>(
        device,
        uploadBatch,
        L"../Modeling/font/gyeong.spritefont",
        cpuHandle,
        gpuHandle
    );

    // GPU 복사 대기
    auto uploadFinished = uploadBatch.End(commandQueue);
    uploadFinished.wait();
}

void CTextRenderer::AddTextInstance(const std::wstring& str, const XMFLOAT4X4& world, const XMFLOAT4& color, bool billboard)
{
    textes.push_back({ str, world, color, billboard });
}

void CTextRenderer::Render(ID3D12GraphicsCommandList* cmdList)
{
    if (textes.empty()) return;

    CScene* scene = CSceneManager::GetInstance().GetActiveScene();
    if (!scene) return;
    auto& camera = scene->GetCamera();
    XMMATRIX view = XMLoadFloat4x4(&camera->GetViewMatrix());
    XMMATRIX proj = XMLoadFloat4x4(&camera->GetProjectionMatrix());
    XMMATRIX viewProj = (view * proj);
    D3D12_VIEWPORT viewport = camera->GetViewPort();

    // 2D용 렌더링 (View/Proj 없이)
    sprite_batch->SetViewport(viewport);

    sprite_batch->Begin(cmdList, SpriteSortMode_Deferred);
    for (auto& item : textes) {
        if (!item.is_billboard) {
            XMFLOAT2 pos = XMFLOAT2(item.world_matrix._41, item.world_matrix._42);
            default_font->DrawString(sprite_batch.get(), item.text.c_str(), pos, XMLoadFloat4(&item.color));
        }
    }

    // 3D 빌보드용 렌더링
    for (auto& item : textes) {
        if (item.is_billboard) {
            // 3D 월드 좌표 추출
            XMMATRIX world = XMMatrixIdentity();
            XMVECTOR worldPos = XMVectorSet(item.world_matrix._41, item.world_matrix._42, item.world_matrix._43, 1.0f);

            // 거리 계산
            XMVECTOR cameraPos = XMLoadFloat3(&camera->GetPosition());
            float distance = XMVectorGetX(XMVector3Length(worldPos - cameraPos));

            // 거리에 따른 스케일 계산 (기본 거리 10.0f 기준)
            float baseScale = 0.2f;
            float minScale = 0.05f;
            float finalScale = (10.0f / distance) * baseScale;
            if (finalScale < minScale) finalScale = minScale;

            // 3D 좌표를 화면의 픽셀 좌표(0~1920 등)로 변환
            XMVECTOR screenPos = XMVector3Project(
                worldPos,
                viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height,
                viewport.MinDepth, viewport.MaxDepth,
                proj, view, world
            );

            // 중앙 정렬
            XMVECTOR size = default_font->MeasureString(item.text.c_str());
            XMVECTOR origin = size * 0.5f;

            default_font->DrawString(
                sprite_batch.get(),
                item.text.c_str(),
                screenPos,
                XMLoadFloat4(&item.color),
                0.f,
                origin,
                finalScale
            );
        }
    }
    sprite_batch->End();

    textes.clear();
}

// CRenderer.cpp 맨 아래 추가
template class CRenderer<InstCB>;
template class CRenderer<UIInstCB>;
template class CRenderer<BillboardInstCB>;
template class CRenderer<AniCB>;