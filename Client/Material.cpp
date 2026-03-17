#include "stdafx.h"
#include "Material.h"
#include "Texture.h"
#include "Shader.h"

// Material
void CMaterial::SetTexture(const std::shared_ptr<CTexture>& tex)
{
	texture = tex;
}

std::shared_ptr<CMaterial> CMaterialManager::GetMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex)
{
	auto it = materials.find(name);
	if (it != materials.end())
		return it->second;

	auto mat = std::make_shared<CMaterial>();
	mat->SetTexture(tex);

	// material 생성 시 index 값 저장
	UINT srvIndex = mat->texture->GetDescriptorIndex();
	mat->material.tex_idx = srvIndex;

	materials.emplace(name, mat);
	return mat;
}

std::shared_ptr<CMaterial> CMaterialManager::GetMeterial(const std::string& name)
{
	auto it = materials.find(name);
	if (it != materials.end()) return nullptr;

	return it->second;
}

void CMaterialManager::LoadMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex)
{
	GetMeterial(name, tex);
}