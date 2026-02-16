#include "stdafx.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Object.h"
#include "Collider.h"


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