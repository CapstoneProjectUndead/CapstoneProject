#include "stdafx.h"
#include "UIComponent.h"
#include "Mesh.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Object.h"

CUIComponent::CUIComponent()
    : world_matrix{Matrix4x4::Identity()}
{
}

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
    // 내 위치 계산
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

    // 행렬 생성 수정
    XMMATRIX matScale = XMMatrixScaling(size.x, size.y, 1.0f);

    // Pivot: (0,0) 기준 메쉬라면 (-pivot.x, pivot.y)
    XMMATRIX matPivot = XMMatrixTranslation(-pivot.x, pivot.y, 0.0f);

    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    XMMATRIX world = matScale * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);

    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

void CUIComponent::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!is_enable) return;

    //CUIRenderer::GetInstance().AddInstance({ 1, 1, 0, 1 }, world_matrix);
    for (auto& c : child) {
        c->Render(commandList);
    }
}

void CUIComponent::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    renderer->AddInstance(nullptr, { 1, 1, 0, 1 }, world_matrix, true);

    for (auto& c : child) {
        c->Collect(renderer);
    }
}

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

// CUICanvas
CUICanvas::CUICanvas()
{
    anchor = { 0.0f, 0.0f };
    pivot = { 0.5f, 0.5f };
    relative_pos = { 0.0f, 0.0f };

    float sw = static_cast<float>(GET_CLIENT_WIDTH);
    float sh = static_cast<float>(GET_CLIENT_HEIGHT);
    size = { sw, sh };
}

void CUICanvas::Update(float deltaTime)
{
    Rect parentRect = GetParentRect();

    size.x = parentRect.right;
    size.y = parentRect.bottom;

    rect.left = 0;
    rect.top = 0;
    rect.right = size.x;
    rect.bottom = size.y;

    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

std::shared_ptr<CUICanvas> CUIManager::CreateCanvas()
{
    auto canvas = std::make_shared<CUICanvas>();
    canvases.push_back(canvas);
    return canvas;
}

// CUIImage
void CUIImage::SetColor(XMFLOAT4 c)
{
    MaterialData m{};
    m.albedo = c;
}

void CUIImage::Update(float deltaTime)
{
    Rect parentRect = GetParentRect();

    float anchorX = parentRect.left + (parentRect.Width() * (anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - anchor.y) * 0.5f);

    float finalX = anchorX + relative_pos.x;
    float finalY = anchorY + relative_pos.y;

    rect.left = finalX - (pivot.x * size.x);
    rect.top = finalY - (pivot.y * size.y);
    rect.right = rect.left + size.x;
    rect.bottom = rect.top + size.y;

    // Fill Amount에 따라 실제 가로 크기를 조절한 world_matrix 재생성
    XMMATRIX matScale = XMMatrixScaling(size.x * fill_amount, size.y, 1.0f);
    XMMATRIX matPivot = XMMatrixTranslation(-pivot.x + 0.5f, -pivot.y + 0.5f, 0.0f);

    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    XMMATRIX world = matScale * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);

    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

void CUIImage::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    renderer->AddInstance(nullptr, mat_comp.get(), world_matrix, true);

    for (auto& c : child) {
        c->Collect(renderer);
    }
}

// CBillboardUI
void CBillboardUI::SetTarget(CObject* obj)
{
    target = obj;
}

void CBillboardUI::Update(float deltaTime)
{
    if (target) {
        // 1. 주인의 월드 위치 가져오기
        XMVECTOR ownerPos = XMLoadFloat3(&owner->position);
        XMVECTOR finalPos = ownerPos + XMLoadFloat3(&offset);

        // 2. 이 위치를 world_matrix의 이동 성분으로 넣음
        // (GS에서 사용할 점의 위치가 됨)
        XMMATRIX matWorld = XMMatrixTranslationFromVector(finalPos);
        XMStoreFloat4x4(&world_matrix, matWorld);
    }
}

// CUIManager
void CUIManager::Update(float deltaTime)
{
    for (auto& canvas : canvases) {
        canvas->Update(deltaTime);
    }
}

void CUIManager::Render(ID3D12GraphicsCommandList* commandList)
{
    for (auto& canvas : canvases) {
        canvas->Render(commandList);
    }
}

void CUIManager::Collect(IRenderer* renderer)
{
    for (auto& canvas : canvases) {
        canvas->Collect(renderer);
    }
}