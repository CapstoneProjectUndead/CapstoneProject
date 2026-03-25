#pragma once
#include "Component.h"

class CMaterialComponent;
class IRenderer;

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

    virtual void Update(const float deltaTime) override;

    virtual void Render(ID3D12GraphicsCommandList* commandList) override;
    virtual void Collect(IRenderer* renderer);
    // getter
    Rect GetParentRect();
    virtual std::string GetShaderName() const { return "ui"; }
    std::string& GetName() { return name; }
    std::vector<std::shared_ptr<CUIComponent>>& GetChildren() { return child; }
    // imgui를 통해 수정
    XMFLOAT2& GetRelativePos() { return relative_pos; }
    XMFLOAT2& GetSize() { return size; }
    XMFLOAT2& GetPivot() { return pivot; }
    XMFLOAT2& GetAnchor() { return anchor; }
    XMFLOAT4& GetColor() { return color; }

    // setter
    void AddChild(std::shared_ptr<CUIComponent> newChild) {
        newChild->parent_ui = this; // 부모 연결
        child.push_back(newChild);
    }
    void SetSize(const XMFLOAT2& s) { size = s; }
    void SetPivot(const XMFLOAT2& p) { pivot = p; }
    void SetRelativePos(const XMFLOAT2& p) { relative_pos = p; }
    void SetName(const std::string& n) { name = n; }

    // Collect 후 자식 순회
    void Traverse(std::map<std::string, std::unique_ptr<IRenderer>>& renderers);
    // Rect에 마우스가 있는지 검사
    virtual bool IntersectsMouse(float x, float y);
protected:
    std::string name{ "UI_Element" };
    EButtonState state{ EButtonState::Disabled };
    XMFLOAT2 relative_pos{ .0f, .0f };
	XMFLOAT2 size{ 100.0f, 100.0f };
	XMFLOAT2 pivot{ 1.0f, 1.0f };	// 본인 기준(0 ~ 1)
	XMFLOAT2 anchor{ 0.0f, 0.0f };	// 부모기준 UI 정렬 기준. (-1 ~ 1)
    XMFLOAT4 color{1, 1, 0, 1};

	XMFLOAT2 final_screen_pos{ .0f, .0f };
    Rect rect{ relative_pos.x - size.x / 2, relative_pos.y - size.y / 2};
    std::vector<std::shared_ptr<CUIComponent>> child;
    CUIComponent* parent_ui{ nullptr }; // 부모 UI 참조 (순환 참조 방지를 위해 생포인터)

    XMFLOAT4X4 world_matrix; // UI의 위치, 크기, 회전이 담긴 행렬
};

class CUICanvas : public CUIComponent
{
public:
    CUICanvas();

    void Update(float deltaTime) override;
    bool IntersectsMouse(float x, float y) override;
};

class CUIImage : public CUIComponent {
public:
    CUIImage();
    float GetFillAmount() const { return fill_amount; }
    void SetFillAmount(float amt) { fill_amount = std::clamp(amt, 0.0f, 1.0f); }
    void SetColor(XMFLOAT4 c);
    void SetMaterial(std::shared_ptr<CMaterialComponent>& m);
    void SetBindingKey(const std::string& key) { binding_key = key; }

    // fill_amount 적용 외에 UIComponent와 유사
    virtual void Update(float deltaTime) override;
    virtual void Collect(IRenderer* renderer) override;

    virtual json Serialize() override;
    virtual void Deserialize(const json& j) override;
protected:
    std::string binding_key = ""; // ex) "PlayerHP"
    float fill_amount = 1.0f;
    std::shared_ptr<CMaterialComponent> mat_comp;  // 생성 시 heap 0 index 사용
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
    virtual void Update(float deltaTime) override;

    // 빌보드를 띄울 타겟(말하는 주체)
    void SetTarget(CObject* obj);
    virtual std::string GetShaderName() const override { return "billboard"; }
private:
    XMFLOAT3 offset{ 0.0f, 1.0f, 0.0f }; // 머리 위 높이 조절
    CObject* target{};
};

class CUIText : public CUIComponent {
public:
    void SetText(const std::wstring& text);
    virtual void Collect(IRenderer* renderer) override;
    virtual std::string GetShaderName() const override { return "text"; }
private:
    std::wstring text{ L"안녕" }; // 서명있는 유니코드로 저장
    bool is_billboard{};
};

class CUIButton : public CUIImage {
public:
    CUIButton();
    virtual void Update(float deltaTime) override;
    virtual void Collect(IRenderer* renderer) override;

    virtual json Serialize() override;
private:
    void GetColorByState();
    void UpdateState();
};

class CUIManager
{
public:
    // 새로운 캔버스 생성
    std::shared_ptr<CUICanvas> CreateCanvas();
    void Update(float deltaTime);
    void Render(ID3D12GraphicsCommandList* commandList);
    void Collect(std::map<std::string, std::unique_ptr<IRenderer>>& renderers);

    // 마우스 클릭 등의 이벤트 처리
    bool IntersectsMouse();
private:
    std::vector<std::shared_ptr<CUICanvas>> canvases;
};