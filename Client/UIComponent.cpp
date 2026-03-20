#include "stdafx.h"
#include "UIComponent.h"
#include "Mesh.h"
#include "GameFramework.h"
#include "Object.h"
#include <pix.h>

bool CUIComponent::Rect::IsPointInside(float x, float y) const
{
    return (x >= left && x <= right && y >= top && y <= bottom);
}

bool CUIComponent::Rect::Intersects(const Rect& other) const
{
    return !(left > other.right || right < other.left ||
        top > other.bottom || bottom < other.top);
}

void CUIComponent::Update(const float deltaTime)
{
    // [A] 내 위치 계산
    Rect parentRect = GetParentRect();

    // Anchor: 부모 내 기준점 (-1 ~ 1 -> 0 ~ 1 변환 후 좌표 산출)
    float anchorX = parentRect.left + (parentRect.Width() * (anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - anchor.y) * 0.5f);

    float finalX = anchorX + relative_pos.x;
    float finalY = anchorY + relative_pos.y;

    // 내 Rect 갱신 (자식들이 나중에 참조할 값)
    rect.left = finalX - (pivot.x * size.x);
    rect.top = finalY - (pivot.y * size.y);
    rect.right = rect.left + size.x;
    rect.bottom = rect.top + size.y;

    // [B] 행렬 생성 수정
    XMMATRIX matScale = XMMatrixScaling(size.x, size.y, 1.0f);

    // Pivot: (0,0) 기준 메쉬라면 (-pivot.x, pivot.y)
    XMMATRIX matPivot = XMMatrixTranslation(-pivot.x, pivot.y, 0.0f);

    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    XMMATRIX world = matScale * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);

    // [C] 자식들 업데이트 (부모인 나의 rect가 갱신된 후 호출되어야 함)
    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

void CUIComponent::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!is_enable) return;

    CUIRenderer::GetInstance().AddInstance({ 1, 1, 0, 1 }, world_matrix);
    for (auto& c : child) {
        c->Render(commandList);
    }
}

// UIComponent.h 또는 .cpp 에 추가
CUIComponent::Rect CUIComponent::GetParentRect()
{
    if (parent_ui) {
        return parent_ui->rect;
    }
    // 화면 전체(Canvas)를 부모 영역으로 간주함
    float screenWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float screenHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    // UI 좌표계: 좌상단(0,0), 우하단(width, height)
    return Rect{ 0.0f, 0.0f, screenWidth, screenHeight };
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