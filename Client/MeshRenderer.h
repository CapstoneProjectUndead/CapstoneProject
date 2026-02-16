#pragma once
#include "Component.h"

class CMesh;
struct FrameNode;
struct MeshCollider;

class CMeshComponent : public CComponent
{
public:
    void SetMesh(std::shared_ptr<CMesh>& m);

    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
    void ReleaseUploadBuffer() override;

    // LoadFrame 정보 Set, T: Vertex type
    template<typename T>
    void SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<FrameNode>& node);
    template<typename T>
    void CMeshComponent::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshCollider& meshData);
private:
    std::shared_ptr<CMesh> mesh;
};

class CMeshRendererComponent : public CComponent
{
public:
    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
};