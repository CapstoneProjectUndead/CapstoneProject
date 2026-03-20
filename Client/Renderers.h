#pragma once
#include "Material.h"

class CMesh;
class CRectangleMesh;

struct InstCB {
    XMFLOAT4X4 world_matrix;
    MaterialData material;
    //UINT bone_offset;
};

class CInstRenderer
{
private:
    CInstRenderer() = default;
    CInstRenderer(const CInstRenderer&) = delete;
public:
    static CInstRenderer& GetInstance() {
        static CInstRenderer instance;
        return instance;
    }
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT instSize = 100);
    void ResizeBuffer(UINT requiredSize);

    void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world);

    void Render(ID3D12GraphicsCommandList* commandList);
private:
    InstCB* mapped{};
    ComPtr<ID3D12Resource> inst_cb;
    UINT max_capacity{};
    ID3D12Device* device{};

    struct BatchKey {
        CMesh* mesh;
        CMaterialComponent* material;
        bool operator<(const BatchKey& other) const {
            return std::tie(mesh, material) < std::tie(other.mesh, other.material);
        }
    };
    // 절대 움직이지 않는 배경용 (매 프레임 clear 안 함!)
    std::map<BatchKey, std::vector<InstCB>> static_batches;

    // 플레이어, 몬스터 등 움직이는 객체용 (매 프레임 clear 함)
    std::map<BatchKey, std::vector<InstCB>> dynamic_batches;
};

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