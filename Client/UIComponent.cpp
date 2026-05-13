#include "stdafx.h"
#include "UIComponent.h"
#include "Mesh.h"
#include "Renderers.h"
#include "GameFramework.h"
#include "Object.h"
#include "KeyManager.h"
#include "DataManager.h"

CUIComponent::CUIComponent()
    : world_matrix{Matrix4x4::Identity()}
{
}

json CUIComponent::Serialize()
{
    json j;
    j["Type"] = "Base"; // 하위 클래스에서 오버라이드
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

// 2. 공통 행렬 계산 로직
void CUIComponent::CalculateWorldMatrix()
{
    Rect parentRect = GetParentRect();

    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);
    float finalX = anchorX + transform.relative_pos.x;
    float finalY = anchorY + transform.relative_pos.y;

    float depth = 0.1f;
    if (parent_ui) {
        depth = parent_ui->GetWorldMatrix()._43 - 0.001f;
    }

    XMMATRIX matScale = XMMatrixScaling(transform.size.x, transform.size.y, 1.0f);
    float pivotOffsetX = (0.5f - transform.pivot.x) * transform.size.x;
    float pivotOffsetY = (0.5f - transform.pivot.y) * transform.size.y;
    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, depth);

    XMStoreFloat4x4(&world_matrix, matScale * matPivot * matTranslation);

    // 판정용 Rect 갱신
    rect.left = finalX - (transform.pivot.x * transform.size.x);
    rect.top = finalY - (transform.pivot.y * transform.size.y);
    rect.right = rect.left + transform.size.x;
    rect.bottom = rect.top + transform.size.y;
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
    transform.anchor = { 0.0f, 0.0f };
    transform.pivot = { 0.0f, 0.0f };
    transform.relative_pos = { 0.0f, 0.0f };

    float sw = static_cast<float>(GET_CLIENT_WIDTH);
    float sh = static_cast<float>(GET_CLIENT_HEIGHT);
    transform.size = { sw, sh };
}

void CUICanvas::CalculateWorldMatrix()
{
    Rect parentRect = GetParentRect();
    transform.size.x = parentRect.right;
    transform.size.y = parentRect.bottom;

    rect.left = 0;
    rect.top = 0;
    rect.right = transform.size.x;
    rect.bottom = transform.size.y;
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

CUIManager::CUIManager()
    : data_manager{std::make_shared<CDataManager>()}
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

// CUIImage
void CUIImage::SetColor(XMFLOAT4 c)
{
    color = c;
}

void CUIImage::SetMaterial(std::shared_ptr<CMaterialComponent>& m)
{
    mat_comp = m;
}

void CUIImage::CalculateWorldMatrix()
{
    Rect parentRect = GetParentRect();

    // 앵커 기반의 기본 위치 계산
    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);

    // 피벗을 고려한 최종 위치 보정
    float finalX = anchorX + transform.relative_pos.x - (transform.pivot.x * transform.size.x);
    float finalY = anchorY + transform.relative_pos.y - (transform.pivot.y * transform.size.y);

    float depth = parent_ui ? parent_ui->GetWorldMatrix()._43 - 0.001f : 0.1f;

    // 행렬 연산: 스케일(fill_amount 적용) -> 위치 이동
    XMMATRIX matScale = XMMatrixScaling(transform.size.x * fill_amount, transform.size.y, 1.0f);
    XMMATRIX matTranslation = XMMatrixTranslation(finalX + (transform.size.x * fill_amount * 0.5f),
        finalY + (transform.size.y * 0.5f), depth);

    XMStoreFloat4x4(&world_matrix, matScale * matTranslation);

    // Rect 갱신
    rect.left = finalX;
    rect.top = finalY;
    rect.right = rect.left + (transform.size.x * fill_amount);
    rect.bottom = rect.top + transform.size.y;
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
        // 이전 값과 다를 때만 강제로 더티 플래그를 키기
        if (fill_amount != *value_ptr) {
            fill_amount = *value_ptr;
            Invalidate();
        }
    }

    // 부모의 업데이트 로직 호출 (이 안에서 is_dirty 체크 후 계산 수행)
    CUIComponent::Update(deltaTime);
}

void CUIImage::Collect(std::unique_ptr<IRenderer>& renderer)
{
    if (!is_enable) return;

    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, mat_comp.get(), world_matrix, 0, true);
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
    // 빌보드의 경우 빌보드에서 위치 Update
    if(!is_billboard)
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
    // 클릭 시 한 번에 다 보여주기
    current_text = full_text;
    current_index = (UINT)full_text.length();
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
    // 머티리얼의 albedo를 버튼 상태 색상으로 업데이트unit.submesh_index
    mat_comp->GetMaterial()->material.albedo = color;
    renderer->AddInstance(nullptr, mat_comp.get(), world_matrix, 0, true);
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
            OnMouseLButtonDown();
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
void CUIDowsingArrow::CalculateWorldMatrix()
{
    float angle = *target_angle;

    float orbitRadius = 250.0f;

    // 원 위에 위치
    transform.relative_pos.x = sinf(angle) * orbitRadius;
    transform.relative_pos.y = -cosf(angle) * orbitRadius;

    Rect parentRect = GetParentRect();
    float anchorX = parentRect.left + (parentRect.Width() * (transform.anchor.x + 1.0f) * 0.5f);
    float anchorY = parentRect.top + (parentRect.Height() * (1.0f - transform.anchor.y) * 0.5f);

    float finalX = anchorX + transform.relative_pos.x;
    float finalY = anchorY + transform.relative_pos.y;

    // 행렬 조립
    XMMATRIX matScale = XMMatrixScaling(transform.size.x, transform.size.y, 1.0f);
    XMMATRIX matRot = XMMatrixRotationZ(angle);

    // 피벗 (중심 회전)
    float pivotOffsetX = (0.5f - transform.pivot.x) * transform.size.x;
    float pivotOffsetY = (0.5f - transform.pivot.y) * transform.size.y;
    XMMATRIX matPivot = XMMatrixTranslation(pivotOffsetX, pivotOffsetY, 0.0f);

    XMMATRIX matTranslation = XMMatrixTranslation(finalX, finalY, 0.1f);

    XMMATRIX world = matScale * matRot * matPivot * matTranslation;
    XMStoreFloat4x4(&world_matrix, world);
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

void CUIManager::ToggleUI(const std::string& name, bool enable, bool setMouseMode)
{
    auto ui = GetUI<CUIComponent>(name);
    if (ui && ui->is_enable != enable) {
        ui->SetEnable(enable);

        CKeyManager::GetInstance().SetMouseMode(setMouseMode);
    }
}