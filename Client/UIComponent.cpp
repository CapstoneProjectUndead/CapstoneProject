#include "stdafx.h"
#include "UIComponent.h"
#include "Mesh.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Object.h"
#include "KeyManager.h"

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
    Rect parentRect = GetParentRect();

    // 기준점(Anchor + Relative) 계산
    float anchorX = parentRect.left + (parentRect.Width() * (anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - anchor.y) * 0.5f);

    float finalX = anchorX + relative_pos.x;
    float finalY = anchorY + relative_pos.y;

    // 판정용 Rect 갱신 (피벗 반영)
    // pivot == 0: left -> finalX가 되고, pivot == 1: right -> finalX
    rect.left = finalX - (pivot.x * size.x);
    rect.top = finalY - (pivot.y * size.y);
    rect.right = rect.left + size.x;
    rect.bottom = rect.top + size.y;

    // Scale 생성
    XMMATRIX matScale = XMMatrixScaling(size.x, size.y, 1.0f);

    // Pivot 보정
    float pivotOffsetX = (0.5f - pivot.x) * size.x;
    float pivotOffsetY = (0.5f - pivot.y) * size.y;

    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);

    // 최종 위치 이동
    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    // Scale -> Pivot -> Translation
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

void CUIComponent::Traverse(std::map<std::string, std::unique_ptr<IRenderer>>& renderers)
{
    std::string shaderKey = GetShaderName(); // 아래 3번 참고
    auto it = renderers.find(shaderKey);

    if (it != renderers.end()) {
        Collect(it->second.get());
    }

    for (auto& c : child) {
        c->Traverse(renderers);
    }
}

bool CUIComponent::IntersectsMouse(float x, float y)
{
    if (state == EButtonState::Disabled) return false;
    return rect.IsPointInside(x, y);
}

void CUIComponent::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    renderer->AddInstance(nullptr, color, world_matrix, true);

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

bool CUICanvas::IntersectsMouse(float x, float y)
{
    // 리스트의 뒤에 있는 자식이 가장 앞에 그려지므로 rbegin()으로 역순 순회
    for (auto it = child.rbegin(); it != child.rend(); ++it) {
        if ((*it)->IntersectsMouse(x, y)) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<CUICanvas> CUIManager::CreateCanvas()
{
    auto canvas = std::make_shared<CUICanvas>();
    canvases.push_back(canvas);
    return canvas;
}

CUIImage::CUIImage()
    : CUIComponent(), mat_comp{ std::make_shared<CMaterialComponent>() }
{
    mat_comp->SetMaterial(std::make_shared<CMaterial>());
}

// CUIImage
void CUIImage::SetColor(XMFLOAT4 c)
{
    color = c;
}

void CUIImage::SetMaterial(std::shared_ptr<CMaterialComponent>& m)
{
    mat_comp = m;
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

    // Scale 생성
    XMMATRIX matScale = XMMatrixScaling(size.x * fill_amount, size.y, 1.0f);

    // Pivot 보정
    float pivotOffsetX = (0.5f - pivot.x) * size.x;
    float pivotOffsetY = (0.5f - pivot.y) * size.y;

    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);

    // 최종 위치 이동
    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    // Scale -> Pivot -> Translation
    XMMATRIX world = matScale * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);
    
    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

void CUIImage::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, mat_comp.get(), world_matrix, true);

    for (auto& c : child) {
        c->Collect(renderer);
    }
}

// CUIBillboard
void CUIBillboard::SetTarget(CObject* obj)
{
    target = obj;
}

void CUIBillboard::Update(float deltaTime)
{
    if (target) {
        // 타겟(플레이어 등)의 월드 위치 + 오프셋
        XMVECTOR targetPos = XMLoadFloat3(&target->position);
        XMVECTOR finalPos = targetPos + XMLoadFloat3(&offset);

        // GS에서 사용할 수 있게 world_matrix의 이동 행렬만 생성
        XMStoreFloat4x4(&world_matrix, XMMatrixTranslationFromVector(finalPos));
    }
}

// CUIText
void CUIText::SetText(const std::wstring& t)
{
    text = t;
}

void CUIText::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    auto textRenderer = dynamic_cast<CTextRenderer*>(renderer);
    if (textRenderer) {
        textRenderer->AddTextInstance(text, world_matrix, color, is_billboard);
    }

    for (auto& c : child) {
        c->Collect(renderer);
    }
}

void CUIButton::Update(float deltaTime)
{
    UpdateState();
    CUIComponent::Update(deltaTime);
}

// CUIButton
void CUIButton::Collect(IRenderer* renderer)
{
    if (!is_enable) return;

    GetColorByState();
    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, color, world_matrix, false);

    for (auto& c : child) {
        c->Collect(renderer);
    }
}

void CUIButton::GetColorByState()
{
    switch (state) {
    case EButtonState::Hover:
        color = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
        break;
    case EButtonState::Pressed:
        color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
        break;
    default:
        color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        break;
    }
}

void CUIButton::UpdateState()
{
    if (state == EButtonState::Disabled) return;

    // 마우스 포인터가 버튼 영역(Rect) 안에 있는지 검사
    CKeyManager& keyManager = CKeyManager::GetInstance();
    Vec2 mouseDelta = keyManager.GetMousePos();
    if (KEY_PRESSED(KEY::LBTN)) {
        if(rect.IsPointInside(mouseDelta.x, mouseDelta.y)) {
            state = EButtonState::Pressed;
        }
    }
    else {
        if (rect.IsPointInside(mouseDelta.x, mouseDelta.y)) {
            state = EButtonState::Hover;
        }
        else {
            state = EButtonState::Normal;
        }
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

void CUIManager::Collect(std::map<std::string, std::unique_ptr<IRenderer>>& renderers)
{
    for (auto& canvas : canvases) {
        // 캔버스부터 시작해서 재귀적으로 수집 시작
        canvas->Traverse(renderers);
    }
}

bool CUIManager::IntersectsMouse()
{
    Vec2 mousePos = CKeyManager::GetInstance().GetMousePos();
    for (auto& canvas : canvases) {
        if (canvas->IntersectsMouse(mousePos.x, mousePos.y)) {
            return true;
        }
    }
    return false;
}