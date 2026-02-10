#pragma once
#include "Component.h"

class CTexture;
class CMesh;
struct FrameNode;

class CMaterial
{
public:
    CMaterial() = default;
    void SetTexture(const std::shared_ptr<CTexture>& tex);
    void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
    void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
public:
    XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };	// 프레넬 효과 반사양
    float glossiness{ 0.25f };

    std::shared_ptr<CTexture> texture;
    ComPtr<ID3D12Resource> material_cb;
};

class CMaterialManager
{
public:
    // 없으면 생성
    std::shared_ptr<CMaterial> GetMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex);
private:
    std::unordered_map<std::string, std::shared_ptr<CMaterial>> materials;
};

struct MaterialCB
{
    XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };
    float glossiness{ 0.25f };
};

// Component
class CMaterialComponent : public CComponent
{
public:
    void SetMaterial(std::shared_ptr<CMaterial>& mat) { material = mat; }
    CMaterial* GetMaterial() const { return material.get(); }

    void Update(const float deltaTime) override {};
    void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList) override;
    void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
private:
    std::shared_ptr<CMaterial> material;
};

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
private:
    std::shared_ptr<CMesh> mesh;
};

class CMeshRendererComponent : public CComponent
{
public:
    void Update(const float deltaTime) override {};
    void Render(ID3D12GraphicsCommandList* commandList) override;
};

class CDebugRendererComponent : public CMeshRendererComponent
{
public:
    void Update(const float deltaTime) override {};
};