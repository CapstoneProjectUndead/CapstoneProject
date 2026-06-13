#include "stdafx.h"
#include "UIComponent.h"
#include "Mesh.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Object.h"
#include "KeyManager.h"
#include "DataManager.h"

CUIComponent::CUIComponent()
    : world_matrix{ Matrix4x4::Identity() }
{
}

json CUIComponent::Serialize()
{
    json j;
    j["Type"] = "Base";
    j["Name"] = name;
    j["RelativePos"] = { transform.relative_pos.x, transform.relative_pos.y };
    j["Size"] = { transform.size.x, transform.size.y };
    j["Pivot"] = { transform.pivot.x, transform.pivot.y };
    j["Anchor"] = { transform.anchor.x, transform.anchor.y };
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
    transform.relative_pos = { j["RelativePos"][0], j["RelativePos"][1] };
    transform.size = { j["Size"][0], j["Size"][1] };
    transform.pivot = { j["Pivot"][0], j["Pivot"][1] };
    transform.anchor = { j["Anchor"][0], j["Anchor"][1] };
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

void CUIComponent::Invalidate()
{
    if (is_dirty) return;
    is_dirty = true;
    for (auto& c : child) {
        c->Invalidate();
    }
}

void CUIComponent::CalculateWorldMatrix()
{
    Rect parentRect = GetParentRect();

    float baseWidth = 1920.0f;
    float baseHeight = 1080.0f;

    float currentWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float currentHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    // 현재 해상도와 디자인 해상도의 비율 계산
    float ratioX = currentWidth / baseWidth;
    float ratioY = currentHeight / baseHeight;

    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);

    float finalX = anchorX + (transform.relative_pos.x * ratioX);
    float finalY = anchorY + (transform.relative_pos.y * ratioY);

    float scaledSizeX = transform.size.x * ratioX;
    float scaledSizeY = transform.size.y * ratioY;

    XMMATRIX matScale = XMMatrixScaling(scaledSizeX, scaledSizeY, 1.0f);

    float pivotOffsetX = (0.5f - transform.pivot.x) * scaledSizeX;
    float pivotOffsetY = (0.5f - transform.pivot.y) * scaledSizeY;
    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.0f);

    XMStoreFloat4x4(&world_matrix, matScale * matPivot * matTranslation);

    rect.left = finalX - (transform.pivot.x * scaledSizeX);
    rect.top = finalY - (transform.pivot.y * scaledSizeY);
    rect.right = rect.left + scaledSizeX;
    rect.bottom = rect.top + scaledSizeY;
}

void CUIComponent::Update(const float deltaTime)
{
    if (!is_enable) return;

    if (is_dirty) {
        CalculateWorldMatrix();
        is_dirty = false;
    }

    for (auto& c : child) {
        c->Update(deltaTime);
    }
}

void CUIComponent::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!is_enable) return;

    for (auto& c : child) {
        c->Render(commandList);
    }
}

void CUIComponent::Traverse(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
    if (!is_enable) return;

    if (renderers[GetShaderName()]) {
        Collect(renderers[GetShaderName()]);
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

void CUIComponent::Collect(std::unique_ptr<IRenderer>& renderer)
{
    if (!is_enable) return;

    renderer->AddInstance(nullptr, color, world_matrix, true);
}

CUIComponent::Rect CUIComponent::GetParentRect()
{
    if (auto parent = parent_ui.lock()) {
        return parent->rect;
    }

    // 부모가 없는 최상위 객체라면 디바이스 화면 전체 크기 반환
    float screenWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float screenHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    return Rect{ 0.0f, 0.0f, screenWidth, screenHeight };
}

// CUICanvas
CUICanvas::CUICanvas()
{
    transform.anchor = { 0.0f, 0.0f };
    transform.pivot = { 0.0f, 0.0f };
    transform.relative_pos = { 0.0f, 0.0f };

    float sw = static_cast<float>(GET_CLIENT_WIDTH);
    float sh = static_cast<float>(GET_CLIENT_HEIGHT);
    transform.size = { sw, sh };
}

void CUICanvas::CalculateWorldMatrix()
{
    float currentWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float currentHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = currentWidth;
    rect.bottom = currentHeight;

    XMMATRIX matIdentity = XMMatrixIdentity();
    XMStoreFloat4x4(&world_matrix, matIdentity);
}

bool CUICanvas::IntersectsMouse(float x, float y)
{
    for (auto it = child.rbegin(); it != child.rend(); ++it) {
        if ((*it)->IntersectsMouse(x, y)) {
            return true;
        }
    }
    return false;
}

CUIManager::CUIManager()
    : data_manager{ std::make_shared<CDataManager>() }
{
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

void CUIImage::SetColor(XMFLOAT4 c)
{
    color = c;
}

void CUIImage::SetMaterial(std::shared_ptr<CMaterialComponent>& m)
{
    mat_comp = m;
}

// CUIImage
void CUIImage::CalculateWorldMatrix()
{
    Rect parentRect = GetParentRect();

    float baseWidth = 1920.0f;
    float baseHeight = 1080.0f;

    float currentWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float currentHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    float ratioX = currentWidth / baseWidth;
    float ratioY = currentHeight / baseHeight;

    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);

    float scaledSizeX = transform.size.x * ratioX;
    float scaledSizeY = transform.size.y * ratioY;

    float finalX = anchorX + (transform.relative_pos.x * ratioX) - (transform.pivot.x * scaledSizeX);
    float finalY = anchorY + (transform.relative_pos.y * ratioY) - (transform.pivot.y * scaledSizeY);

    XMMATRIX matScale = XMMatrixScaling(scaledSizeX * fill_amount, scaledSizeY, 1.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(
        finalX + (scaledSizeX * fill_amount * 0.5f),
        finalY + (scaledSizeY * 0.5f),
        0.0f
    );

    XMStoreFloat4x4(&world_matrix, matScale * matTranslation);

    rect.left = finalX;
    rect.top = finalY;
    rect.right = rect.left + (scaledSizeX * fill_amount);
    rect.bottom = rect.top + scaledSizeY;
}

void CUIImage::Update(const float deltaTime)
{
    if (value_getter) {
        float newValue = value_getter();
        if (fill_amount != newValue) {
            fill_amount = std::clamp(newValue, 0.0f, 1.0f);
            Invalidate();
        }
    }
    else if (value_ptr) {
        if (fill_amount != *value_ptr) {
            fill_amount = *value_ptr;
            Invalidate();
        }
    }

    CUIComponent::Update(deltaTime);
}

void CUIImage::Collect(std::unique_ptr<IRenderer>& renderer)
{
    if (!is_enable) return;

    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, mat_comp, world_matrix, 0, true);
}

json CUIImage::Serialize()
{
    json j = CUIComponent::Serialize();
    j["Type"] = "Image";
    j["FillAmount"] = fill_amount;
    j["TextureName"] = texture_name;
    return j;
}

void CUIImage::Deserialize(const json& j)
{
    CUIComponent::Deserialize(j);
    if (j.contains("FillAmount")) fill_amount = j["FillAmount"];
    if (j.contains("TextureName")) texture_name = j["TextureName"];
}

// CUIBillboard 
void CUIBillboard::SetTarget(std::weak_ptr<CObject> obj)
{
    target = obj;
}

void CUIBillboard::Update(float deltaTime)
{
    auto targetPinned = target.lock();
    if (!targetPinned) return;

    XMVECTOR targetPos = XMLoadFloat3(&targetPinned->position);
    XMVECTOR finalPos = targetPos + XMLoadFloat3(&offset);

    XMStoreFloat4x4(&world_matrix, XMMatrixTranslationFromVector(finalPos));

    for (auto& c : child) {
        XMMATRIX parentMat = XMLoadFloat4x4(&world_matrix);
        XMMATRIX childOffset = XMMatrixTranslation(
            c->GetRelativePos().x * 0.01f,
            c->GetRelativePos().y * 0.01f,
            -0.01f
        );

        c->SetWorldMatrix(Matrix4x4::XMMatrixToFloat4x4(childOffset * parentMat));
        c->Update(deltaTime);
    }
}

// CUIText
CUIText::CUIText()
{
    name = "Text_Element";
}

void CUIText::SetText(const std::wstring& t)
{
    full_text = t;       // 전체 대사 저장
    current_text = L"";  // 출력 텍스트 초기화
    current_index = 0;
    timer = 0.0f;
    is_finished = false; // 다시 타이핑 시작
}

void CUIText::Collect(std::unique_ptr<IRenderer>& renderer)
{
    if (!is_enable) return;

    auto textRenderer = dynamic_cast<CTextRenderer*>(renderer.get());
    if (textRenderer) {
        textRenderer->AddTextInstance(current_text, world_matrix, color, is_billboard);
    }
}

void CUIText::Update(float deltaTime)
{
    if (!is_billboard)
        CUIComponent::Update(deltaTime);

    if (is_finished) {
        current_text = full_text;
        return;
    }

    timer += deltaTime;
    if (timer >= typing_speed) {
        if (current_index < full_text.length()) {
            current_text += full_text[current_index++];
            timer = 0.0f;
        }
        else {
            is_finished = true;
            if (onFinished) onFinished(); // 대사 출력 완료 시 콜백
        }
    }
}

json CUIText::Serialize()
{
    json j = CUIComponent::Serialize();
    j["Type"] = "Text";
    j["Text"] = WstringToUtf8(full_text);
    return j;
}

void CUIText::Deserialize(const json& j)
{
    CUIComponent::Deserialize(j);
    if (j.contains("Text")) full_text = Utf8ToWstring(j["Text"]);
}

void CUIText::Skip()
{
    current_text = full_text;
    current_index = static_cast<UINT>(full_text.length());
    is_finished = true;
    if (onFinished) onFinished();
}

// CUIButton
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

void CUIButton::Collect(std::unique_ptr<IRenderer>& renderer)
{
    if (!is_enable) return;

    GetColorByState();
    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, mat_comp, world_matrix, 0, true);
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

    CKeyManager& keyManager = CKeyManager::GetInstance();
    Vec2 mouseDelta = keyManager.GetMousePos();

    if (KEY_PRESSED(KEY::LBTN)) {
        if (rect.IsPointInside(mouseDelta.x, mouseDelta.y)) {
            state = EButtonState::Pressed;
            OnMouseLButtonDown();
        }
    }
    else {
        if (rect.IsPointInside(mouseDelta.x, mouseDelta.y))
            state = EButtonState::Hover;
        else
            state = EButtonState::Normal;
    }
}

void CUIDowsingArrow::CalculateWorldMatrix()
{
    if (!target_angle)
        return;

    float angle = *target_angle;

    float baseWidth = 1920.0f;
    float baseHeight = 1080.0f;

    float currentWidth = static_cast<float>(GET_CLIENT_WIDTH);
    float currentHeight = static_cast<float>(GET_CLIENT_HEIGHT);

    float ratioX = currentWidth / baseWidth;
    float ratioY = currentHeight / baseHeight;

    float orbitRadius = 250.0f * ratioY;

    transform.relative_pos.x = sinf(angle) * orbitRadius;
    transform.relative_pos.y = -cosf(angle) * orbitRadius;

    Rect parentRect = GetParentRect();
    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);

    float finalX = anchorX + transform.relative_pos.x;
    float finalY = anchorY + transform.relative_pos.y;

    float scaledSizeX = transform.size.x * ratioX;
    float scaledSizeY = transform.size.y * ratioY;

    XMMATRIX matScale = XMMatrixScaling(scaledSizeX, scaledSizeY, 1.0f);
    XMMATRIX matRot = XMMatrixRotationZ(angle);

    float pivotOffsetX = (0.5f - transform.pivot.x) * scaledSizeX;
    float pivotOffsetY = (0.5f - transform.pivot.y) * scaledSizeY;
    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.0f);

    XMMATRIX world = matScale * matRot * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);

    rect.left = finalX - (transform.pivot.x * scaledSizeX);
    rect.top = finalY - (transform.pivot.y * scaledSizeY);
    rect.right = rect.left + scaledSizeX;
    rect.bottom = rect.top + scaledSizeY;
}

void CUIDowsingArrow::Update(float deltaTime)
{
    if (!is_enable) return;
    Invalidate();
    CUIImage::Update(deltaTime);
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

void CUIManager::Collect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
    for (auto& canvas : canvases) {
        canvas->Traverse(renderers);
    }
}

bool CUIManager::IntersectsMouse()
{
    Vec2 mousePos = CKeyManager::GetInstance().GetMousePos();
    for (auto& canvas : canvases) {
        if (canvas->IntersectsMouse(mousePos.x, mousePos.y))
            return true;
    }
    return false;
}

void CUIManager::ToggleUI(const std::string& name, bool enable, bool setMouseMode)
{
    auto ui = GetUI<CUIComponent>(name);
    if (ui && ui->is_enable != enable) {
        ui->SetEnable(enable);
        CKeyManager::GetInstance().SetMouseMode(setMouseMode);
    }
}

void CUIManager::Invalidate()
{
    for (auto& canvas : canvases) {
        canvas->Invalidate();
    }
}