#include "stdafx.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Object.h"
#include "Collider.h"
#include "Material.h"

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

	for (auto& unit : render_units) {
		if (unit.material)
			unit.material->UpdateMeshShaderVariables(commandList);
		if (unit.mesh)
			unit.mesh->Render(commandList);
	}
#ifdef DEBUG
	auto collider = owner->GetComponents<CColliderComponent>();
	for (auto c : collider)
		c->Render(commandList);
#endif
}