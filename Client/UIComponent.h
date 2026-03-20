#pragma once
#include "Component.h"

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

    CUIComponent() = default;

    virtual void Update(const float deltaTime) override;

    virtual void Render(ID3D12GraphicsCommandList* commandList) override;
    Rect GetParentRect();
    void AddChild(std::shared_ptr<CUIComponent> newChild) {
        newChild->parent_ui = this; // 부모 연결
        child.push_back(newChild);
    }
protected:
	XMFLOAT2 relative_pos{ .0f, .0f };
	XMFLOAT2 size{ 100.0f, 100.0f };
	XMFLOAT2 pivot{ 0.0f, 0.0f };	// 본인 기준(0 ~ 1)
	XMFLOAT2 anchor{ 0.0f, 0.0f };	// 부모기준 UI 정렬 기준. (-1 ~ 1)

	XMFLOAT2 final_screen_pos{ .0f, .0f };
    Rect rect{ relative_pos.x - size.x / 2, relative_pos.y - size.y / 2};
    std::vector<std::shared_ptr<CUIComponent>> child;
    CUIComponent* parent_ui{ nullptr }; // 부모 UI 참조 (순환 참조 방지를 위해 생포인터)

    XMFLOAT4X4 world_matrix; // UI의 위치, 크기, 회전이 담긴 행렬
};

class CMesh;
class CRectangleMesh;

struct UIInstCB {
    XMFLOAT4X4 world_matrix; // UI의 위치, 크기, 회전이 담긴 행렬
    XMFLOAT4 color;          // 추후에 material로 변경
};

class CUIRenderer {
private:
    CUIRenderer();
    CUIRenderer(const CUIRenderer&) = delete;
public:
    static CUIRenderer& GetInstance() {
        static CUIRenderer instance;
        return instance;
    }
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT instSize = 100);
    void ResizeBuffer(UINT requiredSize);

    void AddInstance(XMFLOAT4 color, const XMFLOAT4X4& world);
    void Render(ID3D12GraphicsCommandList* commandList);
private:
    UIInstCB* mapped{};
    ComPtr<ID3D12Resource> inst_cb;
    UINT max_capacity{};
    std::shared_ptr<CRectangleMesh> quad_mesh;

    std::map<std::shared_ptr<CMesh>, std::vector<UIInstCB>> ui_batches;
};