#include "stdafx.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Object.h"
#include "Texture.h"

// Material
void CMaterial::SetTexture(const std::shared_ptr<CTexture>& tex)
{
	texture = tex;
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!material_cb) return;
	MaterialCB cb{};
	cb.albedo = albedo;
	cb.fresnel = fresnel;
	cb.glossiness = glossiness;

	UINT8* mapped = nullptr;
	material_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &cb, sizeof(cb));
	material_cb->Unmap(0, nullptr);

	commandList->SetGraphicsRootConstantBufferView(2, material_cb->GetGPUVirtualAddress());
}

void CMaterial::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	MaterialCB cb{};
	material_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<MaterialCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
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

// Component
void CMaterialComponent::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!material) return;

	material->UpdateShaderVariables(commandList);
}

void CMaterialComponent::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	if (!material) return;

	material->CreateConstantBuffers(device, commandList);
}

void CMeshComponent::SetMesh(std::shared_ptr<CMesh>& m)
{
	mesh = m;
}

void CMeshComponent::Render(ID3D12GraphicsCommandList* commandList)
{
	if (!mesh) return;

	mesh->Render(commandList);
}

void CMeshComponent::ReleaseUploadBuffer()
{
	if (!mesh) return;

	mesh->ReleaseUploadBuffer();
}

void CMeshRendererComponent::Render(ID3D12GraphicsCommandList* commandList)
{
	if (!owner) return;

	auto meshComp = owner->GetComponent<CMeshComponent>();
	if (meshComp)
		meshComp->Render(commandList);
}