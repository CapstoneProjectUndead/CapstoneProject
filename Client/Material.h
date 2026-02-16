#pragma once
#include "Component.h"

class CTexture;

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
    void UpdateMeshShaderVariables(ID3D12GraphicsCommandList* commandList) override;
    void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;
private:
    std::shared_ptr<CMaterial> material;
};
