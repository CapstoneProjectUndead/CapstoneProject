#include "stdafx.h"
#include "Material.h"
#include "Texture.h"

// Material
void CMaterial::SetTexture(const std::shared_ptr<CTexture>& tex)
{
	texture = tex;
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!material_cb) return;

	memcpy(mapped, &material, sizeof(material));

	commandList->SetGraphicsRootConstantBufferView(2, material_cb->GetGPUVirtualAddress());
}

void CMaterial::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	material_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<MaterialCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	material_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

std::shared_ptr<CMaterial> CMaterialManager::GetMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex)
{
	auto it = materials.find(name);
	if (it != materials.end())
		return it->second;

	auto mat = std::make_shared<CMaterial>();
	mat->SetTexture(tex);

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
	auto it = materials.find(name);
	if (it != materials.end()) return;

	auto mat = std::make_shared<CMaterial>();
	mat->SetTexture(tex);

	materials.emplace(name, mat);
}

// Component
void CMaterialComponent::UpdateMeshShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!material) return;

	material->UpdateShaderVariables(commandList);
}

void CMaterialComponent::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	if (!material) return;

	material->CreateConstantBuffers(device, commandList);
}