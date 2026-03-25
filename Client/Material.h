#pragma once
#include "Component.h"

class CTexture;

struct MaterialData
{
    XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };
    float glossiness{ 0.25f };
    UINT tex_idx;
};

class CMaterial
{
public:
    CMaterial() = default;
    void SetTexture(const std::shared_ptr<CTexture>& tex);
    MaterialData GetMaterialData() const { return material; }
    void SetMaterialData(MaterialData other) { material = other; }
    UINT GetTexIndex() const { return material.tex_idx; };
public:
    MaterialData material{};  // meterial Data

    std::shared_ptr<CTexture> texture;
};

class CMaterialManager
{
public:
    // 없으면 생성
    std::shared_ptr<CMaterial> GetMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex);
    std::shared_ptr<CMaterial> GetMaterial(const std::string& name);
    // 미리 Load
    void LoadMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex);
private:
    std::unordered_map<std::string, std::shared_ptr<CMaterial>> materials;
};

// Component
class CMaterialComponent : public CComponent
{
public:
    CMaterialComponent() = default;
    void SetMaterial(std::shared_ptr<CMaterial>& mat) { material = mat; }
    CMaterial* GetMaterial() const { return material.get(); }

    void Update(const float deltaTime) override {};
private:
    std::shared_ptr<CMaterial> material;
};
