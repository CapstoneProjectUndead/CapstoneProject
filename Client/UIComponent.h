#pragma once
#include "Component.h"

class CMaterialComponent;
class IRenderer;
class CDataManager;

// world_matrix에 적용하는 UI 구조체
struct UITransform {
    XMFLOAT2 relative_pos{ 0.0f, 0.0f };
    XMFLOAT2 size{ 100.0f, 100.0f };
    XMFLOAT2 pivot{ 0.5f, 0.5f };
    XMFLOAT2 anchor{ 0.0f, 0.0f };
};

// UIManager로 관리, 각자의 world_matrix가 존재
class CUIComponent : public CComponent{
public:
	struct Rect {
        float left{ 0.0f };
        float top{ 0.0f };
        float right{ 0.0f };
        float bottom{ 0.0f };

        inline float Width() const { return right - left; }
        inline float Height() const { return bottom - top; }
        inline XMFLOAT2 Center() const { return { (left + right) * 0.5f, (top + bottom) * 0.5f }; }
        // 마우스 클릭 판정
        bool IsPointInside(float x, float y) const;
        // 두 사각형이 겹치는지 확인 (UI 레이아웃 겹침 방지 등)
        bool Intersects(const Rect& other) const;
    };
    enum class EButtonState { Normal, Hover, Pressed, Disabled };

    CUIComponent();

    // json data
    virtual json Serialize();
    virtual void Deserialize(const json& j);

    // 가상 함수로 분리된 행렬 계산 로직
    virtual void CalculateWorldMatrix();
    // Dirty Flag 전파용
    void Invalidate();
    // Flag 확인과 자식 전파만 담당
    virtual void Update(const float deltaTime) override;

    virtual void Render(ID3D12GraphicsCommandList* commandList) override;
    virtual void Collect(IRenderer* renderer);
    // getter
    Rect GetParentRect();
    virtual std::string GetShaderName() const { return "ui"; }
    std::string& GetName() { return name; }
    std::vector<std::shared_ptr<CUIComponent>>& GetChildren() { return child; }
    // imgui를 통해 수정
    XMFLOAT2 GetRelativePos() { return transform.relative_pos; }
    XMFLOAT2 GetSize() { return transform.size; }
    XMFLOAT2 GetPivot() { return transform.pivot; }
    XMFLOAT2 GetAnchor() { return transform.anchor; }
    XMFLOAT4 GetColor() { return color; }
    XMFLOAT4X4 GetWorldMatrix() const { return world_matrix; }

    // setter
    void AddChild(std::shared_ptr<CUIComponent> newChild) {
        newChild->parent_ui = this; // 부모 연결
        child.push_back(newChild);
    }
    void SetRelativePos(const XMFLOAT2& p) { transform.relative_pos = p; Invalidate(); }
    void SetSize(const XMFLOAT2& s) { transform.size = s; Invalidate(); }
    void SetPivot(const XMFLOAT2& p) { transform.pivot = p; Invalidate(); }
    void SetAnchor(const XMFLOAT2& a) { transform.anchor = a; Invalidate(); }
    void SetWorldMatrix(const XMFLOAT4X4& m) { world_matrix = m; Invalidate(); }
    void SetName(const std::string& n) { name = n; }
    void SetColor(const XMFLOAT4& c) { color = c; }

    // Collect 후 자식 순회
    void Traverse(std::map<std::string, std::unique_ptr<IRenderer>>& renderers);
    // Rect에 마우스가 있는지 검사
    virtual bool IntersectsMouse(float x, float y);
protected:
    std::string name{ "UI_Element" };
    EButtonState state{ EButtonState::Disabled };
    UITransform transform{};
    XMFLOAT4 color{1, 1, 1, 1};

	XMFLOAT2 final_screen_pos{ .0f, .0f };
    Rect rect{ transform.relative_pos.x - transform.size.x / 2, transform.relative_pos.y - transform.size.y / 2};
    std::vector<std::shared_ptr<CUIComponent>> child;
    CUIComponent* parent_ui{ nullptr }; // 부모 UI 참조 (순환 참조 방지를 위해 생포인터)

    bool is_dirty{ true };
    XMFLOAT4X4 world_matrix; // UI의 위치, 크기, 회전이 담긴 행렬
};

class CUICanvas : public CUIComponent
{
public:
    CUICanvas();
    virtual void CalculateWorldMatrix() override;
    bool IntersectsMouse(float x, float y) override;
};

class CUIImage : public CUIComponent {
public:
    CUIImage();
    float GetFillAmount() const { return fill_amount; }
    void SetFillAmount(float amt) { fill_amount = std::clamp(amt, 0.0f, 1.0f); }
    void SetColor(XMFLOAT4 c);
    void SetMaterial(std::shared_ptr<CMaterialComponent>& m);

    // fill_amount 적용 외에 UIComponent와 유사
    virtual void CalculateWorldMatrix() override;
    virtual void Update(const float deltaTime) override;
    virtual void Collect(IRenderer* renderer) override;

    virtual json Serialize() override;
    virtual void Deserialize(const json& j) override;

    void SetTextureName(const std::string& name) { texture_name = name; }
    const std::string& GetTextureName() const { return texture_name; }

    void BindFillAmount(float* valuePtr) { value_ptr = valuePtr; }
    void BindFillAmount(std::function<float()> getter) { value_getter = getter; }
protected:
    float fill_amount = 1.0f;
    float* value_ptr{}; // fill_amount 참조 값
    std::function<float()> value_getter;
    std::shared_ptr<CMaterialComponent> mat_comp;  // 생성 시 heap 0 index 사용
    std::string texture_name{ "white" }; // 텍스처 파일 이름 저장(load/save 용)
};

/*
* 사용 예시
auto billboard = std::make_shared<CUIBillboard>(obj.get());
std::shared_ptr<CMaterialComponent> m = std::make_shared<CMaterialComponent>();
m->SetMaterial(factory->GetMaterial(shaders["billboard"]->GetHeapManager(), "white"));
billboard->SetMaterial(m);
mainCanvas->AddChild(billboard);
*/
class CUIBillboard : public CUIImage {
public:
    CUIBillboard(CObject* obj) :target{obj} {}
    // 타겟 위치로 Update. 자식의 위치 업데이트도 수행
    virtual void Update(float deltaTime) override;

    // 빌보드를 띄울 타겟(말하는 주체)
    void SetTarget(CObject* obj);
    virtual std::string GetShaderName() const override { return "billboard"; }
private:
    XMFLOAT3 offset{ 1.0f, 1.0f, -0.1f }; // 머리 위 높이 조절
    CObject* target{};
};

class CUIText : public CUIComponent {
public:
    CUIText();

    virtual std::string GetShaderName() const override { return "text"; }
    void SetText(const std::wstring& text);
    void SetBillboard(bool b) { is_billboard = b; }
    virtual void Collect(IRenderer* renderer) override;
    virtual void Update(float deltaTime) override;
    virtual json Serialize() override;
    virtual void Deserialize(const json& j) override;

    bool IsFinished() const { return is_finished; }
    void Skip();

    std::function<void()> onFinished{}; // 버튼 띄우기용 콜백
private:
    std::wstring full_text{ L"Hi" };
    std::wstring current_text{}; // 서명있는 유니코드로 저장
    UINT current_index{};
    float timer{};
    float typing_speed{ 0.3f }; // 글자당 속도
    bool is_finished{true};
    bool is_billboard{};
};

class CUIButton : public CUIImage {
public:
    CUIButton();
    virtual void Update(float deltaTime) override;
    virtual void Collect(IRenderer* renderer) override;

    virtual json Serialize() override;

    std::function<void()> OnClick;
    void OnMouseLButtonDown() {
        if (OnClick) OnClick();
    }
private:
    void GetColorByState();
    void UpdateState();
};

// 비활성화를 위해 owner가 필요
class CUIDowsingArrow : public CUIImage {
public:
    CUIDowsingArrow(float* angle) : CUIImage(), target_angle{angle}
    { name = "Dowsing_Arrow"; }

    // 외부에서 계산한 각도를 주입 (라디안 값)
    void SetTargeAngle(float* radian) { target_angle = radian; }

    virtual void CalculateWorldMatrix() override;
    virtual void Update(float deltaTime) override;
private:
    float* target_angle{}; // 현재 가리킬 각도
};

class CUIManager
{
public:
    std::shared_ptr<CDataManager>& GetDataManager() { return data_manager; }

    // 새로운 캔버스 생성
    CUIManager();

    std::shared_ptr<CUICanvas> CreateCanvas();
    void AddCanvas(std::shared_ptr<CUICanvas>& canvas) {
        canvases.push_back(canvas);
    }
    std::vector<std::shared_ptr<CUICanvas>>& GetCanvases() { return canvases; }
    void Update(float deltaTime);
    void Render(ID3D12GraphicsCommandList* commandList);
    void Collect(std::map<std::string, std::unique_ptr<IRenderer>>& renderers);

    // 마우스 클릭 등의 이벤트 처리
    bool IntersectsMouse();

    // cache
    // UI를 등록하여 캐시하는 함수
    void RegisterUI(const std::string& name, std::shared_ptr<CUIComponent> ui) {
        ui_cache[name] = ui;
    }

    // 특정 이름의 UI를 찾는 기능
    template <typename T>
    std::shared_ptr<T> FindUI(const std::string& name) {
        for (auto& canvas : canvases) {
            auto found = FindRecursive<T>(canvas, name);
            if (found) return found;
        }
        return nullptr;
    }

    // 캐시된 UI를 즉시 반환 (없으면 찾아보고 캐싱)
    template <typename T>
    std::shared_ptr<T> GetUI(const std::string& name) {
        if (ui_cache.find(name) != ui_cache.end())
            return std::dynamic_pointer_cast<T>(ui_cache[name]);

        auto found = FindUI<T>(name);
        if (found) ui_cache[name] = found;
        return found;
    }

    // 특정 UI를 껐다 켰다 하는 관리 함수
    void ToggleUI(const std::string& name, bool enable, bool setMouseMode = false);
private:
    template <typename T>
    std::shared_ptr<T> FindRecursive(std::shared_ptr<CUIComponent> parent, const std::string& name) {
        if (parent->GetName() == name) return std::dynamic_pointer_cast<T>(parent);
        for (auto& c : parent->GetChildren()) {
            auto found = FindRecursive<T>(c, name);
            if (found) return found;
        }
        return nullptr;
    }
private:
    std::vector<std::shared_ptr<CUICanvas>> canvases;
    std::shared_ptr<CDataManager> data_manager;
    std::map<std::string, std::shared_ptr<CUIComponent>> ui_cache;
};