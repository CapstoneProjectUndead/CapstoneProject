#include "stdafx.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Object.h"
#include "Collider.h"
#include "Renderers.h"
#include "Animator.h"

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
		if (!unit.mesh->is_enable) continue;
		if (unit.material && !unit.material->is_enable) continue;
	}
}

void CMeshRendererComponent::Collect(std::unique_ptr<IRenderer>& renderer, bool isStatic)
{
	if (!owner) return;

	auto animator = owner->GetComponent<CAnimatorComponent>();
	AnimationData aniData;
	if (animator)
		aniData = animator->GetAnimationData();

	for (auto& unit : render_units) {
		if (!unit.mesh->is_enable) continue;
		if (unit.material && !unit.material->is_enable) continue;

		if (animator) {
			// 애니메이터로부터 현재 프레임 정보를 가져옴
			renderer->AddInstance(unit.mesh->GetMesh(), unit.material, owner->world_matrix, unit.submesh_index, aniData);
		}
		else
			renderer->AddInstance(unit.mesh->GetMesh(), unit.material, owner->world_matrix, unit.submesh_index, isStatic);
	}
}
