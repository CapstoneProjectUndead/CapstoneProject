#pragma once
#include "Component.h"

class CTexture;
class CDescriptorHeapManager;

struct MaterialData
{
    XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };
    float glossiness{ 0.25f };
    XMFLOAT4 emissive_color;
    UINT tex_idx{};
    UINT normal_idx{ UINT_MAX };
};

class CMaterial
{
public:
    CMaterial() = default;
    void SetTexture(const std::shared_ptr<CTexture>& tex);
    std::shared_ptr<CTexture> GetTexture() { return texture; };
    void SetNormalIndex(const std::shared_ptr<CTexture>& tex);
    MaterialData GetMaterialData() const { return material; }
    void SetMaterialData(MaterialData other) { material = other; }
    UINT GetTexIndex() const { return material.tex_idx; };
    UINT GetNormaIndex() const { return material.normal_idx; };
public:
    MaterialData material{};  // meterial Data

    std::shared_ptr<CTexture> texture;
};

class CMaterialManager
{
public:
    // 없으면 생성
    std::shared_ptr<CMaterial> GetMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex, const EShaderName shaderName);
    std::shared_ptr<CMaterial> GetMaterial(const std::string& name, const EShaderName shaderName);
    // 미리 Load
    void LoadMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex, const EShaderName shaderName);
private:
    std::unordered_map<std::string, std::shared_ptr<CMaterial>> materials;
};

// Component
class CMaterialComponent : public CComponent
{
public:
    CMaterialComponent() = default;
    void SetMaterial(std::shared_ptr<CMaterial>& mat) { material = mat; }
    std::shared_ptr<CMaterial> GetMaterial() const { return material; }

    void Update(const float deltaTime) override {};
private:
    std::shared_ptr<CMaterial> material;
};
