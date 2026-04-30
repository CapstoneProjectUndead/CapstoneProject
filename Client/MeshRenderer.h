#pragma once
#include "Component.h"

class CMesh;
class CMaterialComponent;
class IRenderer;

namespace CGeometryLoader {
    struct FrameNode;
    struct MeshCollider;
}

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
    void SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node);
    template<typename T>
    void CMeshComponent::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const CGeometryLoader::MeshCollider& meshData);
private:
    std::shared_ptr<CMesh> mesh;
};

struct RenderUnit
{
    std::shared_ptr<CMeshComponent> mesh;
    std::shared_ptr<CMaterialComponent> material;
};

class CMeshRendererComponent : public CComponent
{
public:
    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
    void Collect(std::unique_ptr<IRenderer>& renderer, bool isStatic);
    void SetRenderUnit(std::shared_ptr<CMeshComponent>& mesh)
    {
        render_units.push_back({ mesh, nullptr });
    }
    void SetRenderUnit(std::shared_ptr<CMeshComponent>& mesh, std::shared_ptr<CMaterialComponent>& mat)
    {
        render_units.push_back({ mesh, mat });
    }
    void SetRenderUnit(RenderUnit renderUnit)
    {
        render_units.push_back(renderUnit);
    }
    std::vector<RenderUnit>& GetRenderUnits() { return render_units; }

    void SetShader(const EShaderName name) { shader_name = name; }
    EShaderName GetShader() const { return shader_name; }
private:
    std::vector<RenderUnit> render_units;
    EShaderName shader_name{ EShaderName::Inst };	// 적용 쉐이더 이름
};
