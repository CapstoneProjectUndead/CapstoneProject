#include "stdafx.h"
#include "Material.h"
#include "Texture.h"
#include "Shader.h"

// Material
void CMaterial::SetTexture(const std::shared_ptr<CTexture>& tex)
{
	texture = tex;

	// index 값 저장
	UINT srvIndex = texture->GetDescriptorIndex();
	material.tex_idx = srvIndex;
}

std::shared_ptr<CMaterial> CMaterialManager::GetMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex, CDescriptorHeapManager* heap)
{
	std::string materialKey = name + "_" + std::to_string((uintptr_t)heap);
	auto it = materials.find(materialKey);
	if (it != materials.end()) return it->second;

	auto mat = std::make_shared<CMaterial>();
	mat->SetTexture(tex);

	materials.emplace(materialKey, mat);
	return mat;
}

std::shared_ptr<CMaterial> CMaterialManager::GetMaterial(const std::string& name, CDescriptorHeapManager* heap)
{
	std::string materialKey = name + "_" + std::to_string((uintptr_t)heap);
	auto it = materials.find(materialKey);
	if (it != materials.end()) return nullptr;

	return it->second;
}

void CMaterialManager::LoadMaterial(const std::string& name, const std::shared_ptr<CTexture>& tex, CDescriptorHeapManager* heap)
{
	GetMaterial(name, tex, heap);
}