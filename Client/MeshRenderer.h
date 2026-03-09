#pragma once
#include "Component.h"

class CMesh;
class CMaterialComponent;
struct FrameNode;
struct MeshCollider;
struct ObjectCB;

class CMeshComponent : public CComponent
{
public:
    void SetMesh(std::shared_ptr<CMesh>& m);

    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
    void ReleaseUploadBuffer() override;

    std::shared_ptr<CMesh>& GetMesh() { return mesh; };
    // LoadFrame 정보 Set, T: Vertex type
    template<typename T>
    void SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<FrameNode>& node);
    template<typename T>
    void CMeshComponent::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshCollider& meshData);
private:
    std::shared_ptr<CMesh> mesh;
};

struct RenderUnit
{
    CMeshComponent* mesh{};
    CMaterialComponent* material{};
};

class CMeshRendererComponent : public CComponent
{
public:
    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
    void SetRenderUnit(CMeshComponent* mesh, CMaterialComponent* mat = nullptr)
    {
        render_units.push_back({ mesh, mat });
    }
    void SetRenderUnit(RenderUnit renderUnit)
    {
        render_units.push_back(renderUnit);
    }
private:
    std::vector<RenderUnit> render_units;
};

struct InstCB {
    XMFLOAT4X4 world_matrix;
    UINT material_idx{};
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

    void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world);

    void Render(ID3D12GraphicsCommandList* commandList);
private:
    InstCB* mapped{};
    ComPtr<ID3D12Resource> inst_cb;

    struct BatchKey {
        CMesh* mesh;
        CMaterialComponent* material;
        bool operator<(const BatchKey& other) const {
            return std::tie(mesh, material) < std::tie(other.mesh, other.material);
        }
    };
    std::map<BatchKey, std::vector<InstCB>> batches;
};