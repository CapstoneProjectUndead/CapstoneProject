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

json CUIComponent::Serialize()
{
    json j;
    j["Type"] = "Base"; // 하위 클래스에서 오버라이드
    j["Name"] = name;
    j["RelativePos"] = { relative_pos.x, relative_pos.y };
    j["Size"] = { size.x, size.y };
    j["Pivot"] = { pivot.x, pivot.y };
    j["Anchor"] = { anchor.x, anchor.y };
    j["Color"] = { color.x, color.y, color.z, color.w };

    j["Children"] = json::array();
    for (auto& c : child) {
        j["Children"].push_back(c->Serialize());
    }
    return j;
}

void CUIComponent::Deserialize(const json& j)
{
    name = j["Name"];
    relative_pos = { j["RelativePos"][0], j["RelativePos"][1] };
    size = { j["Size"][0], j["Size"][1] };
    pivot = { j["Pivot"][0], j["Pivot"][1] };
    anchor = { j["Anchor"][0], j["Anchor"][1] };
    color = { j["Color"][0], j["Color"][1], j["Color"][2], j["Color"][3] };
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
    pivot = { 0.0f, 0.0f };
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
    name = "Image_Element";
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
    if (!binding_key.empty()) {
        // 외부 전역 매니저에서 키에 해당하는 0.0 ~ 1.0 값을 가져옴
        //float value = CDataManager::GetInstance().GetFloat(binding_key);
        //SetFillAmount(value);
    }

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
}

json CUIImage::Serialize()
{
    json j = CUIComponent::Serialize();
    j["Type"] = "Image";
    j["FillAmount"] = fill_amount;
    return j;
}

void CUIImage::Deserialize(const json& j)
{
    CUIComponent::Deserialize(j);
    if (j.contains("FillAmount")) fill_amount = j["FillAmount"];
}

// CUIBillboard
void CUIBillboard::SetTarget(CObject* obj)
{
    target = obj;
}

void CUIBillboard::Update(float deltaTime)
{
    if (!target) return;

    // 빌보드의 기준 3D 위치 계산
    XMVECTOR targetPos = XMLoadFloat3(&target->position);
    XMVECTOR finalPos = targetPos + XMLoadFloat3(&offset);

    // 빌보드 본인의 행렬 갱신
    XMStoreFloat4x4(&world_matrix, XMMatrixTranslationFromVector(finalPos));

    // 자식들 업데이트
    for (auto& c : child) {
        XMMATRIX parentMat = XMLoadFloat4x4(&world_matrix);
        XMMATRIX childOffset = XMMatrixTranslation(
            c->GetRelativePos().x * 0.01f,
            c->GetRelativePos().y * 0.01f,
            -0.01f // 글자가 말풍선보다 살짝 카메라 앞에 오도록 Z값 조절
        );

        // 자식의 최종 3D 월드 행렬 = 자식 오프셋 * 부모(빌보드) 위치
        c->SetWorldMatrix(Matrix4x4::XMMatrixToFloat4x4(childOffset * parentMat));
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
}

CUIButton::CUIButton()
    : CUIImage()
{
    name = "Button_Element";
    state = EButtonState::Normal;
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
}

json CUIButton::Serialize()
{
    json j = CUIImage::Serialize();
    j["Type"] = "Button";
    return j;
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

// CUIDowsingArrow
void CUIDowsingArrow::Update(float deltaTime)
{
    if (!is_enable) return;

    float angle = *target_angle;

    float orbitRadius = 250.0f;

    // 원 위에 위치
    relative_pos.x = sinf(angle) * orbitRadius;
    relative_pos.y = -cosf(angle) * orbitRadius;

    Rect parentRect = GetParentRect();
    float anchorX = parentRect.left + (parentRect.Width() * (anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - anchor.y) * 0.5f);

    float finalX = anchorX + relative_pos.x;
    float finalY = anchorY + relative_pos.y;

    // 행렬 조립
    XMMATRIX matScale = XMMatrixScaling(size.x, size.y, 1.0f);
    XMMATRIX matRot = XMMatrixRotationZ(angle);

    // 피벗 (중심 회전)
    float pivotOffsetX = (0.5f - pivot.x) * size.x;
    float pivotOffsetY = (0.5f - pivot.y) * size.y;
    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);

    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    XMMATRIX world = matScale * matRot * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);

    for (auto& c : child) {
        c->Update(deltaTime);
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

