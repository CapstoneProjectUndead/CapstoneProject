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
		if (!unit.mesh->is_enable) continue;
		if (unit.material && !unit.material->is_enable) continue;

		if (unit.material)
			unit.material->UpdateMeshShaderVariables(commandList);

		unit.mesh->Render(commandList);
#ifdef DEBUG
		auto collider = owner->GetComponents<CColliderComponent>();
		for (auto c : collider)
			c->Render(commandList);
#endif
	}
}

void CInstRenderer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT instSize)
{
	inst_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<ObjectCB>() * instSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	inst_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
}

void CInstRenderer::AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world)
{
	ObjectCB data;
	XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world));
	XMStoreFloat4x4(&data.world_matrix, worldT);

	batches[{mesh, material}].push_back(data);
}

void CInstRenderer::Render(ID3D12GraphicsCommandList* commandList)
{
	UINT currentOffset = 0;

	for (auto& [key, instances] : batches) {
		UINT count = (UINT)instances.size();
		if (count == 0) continue;

		// 1. 데이터 복사
		memcpy(&mapped[currentOffset], instances.data(), sizeof(ObjectCB) * count);

		// 2. 머티리얼 및 텍스처 설정 (Slot 5번 Descriptor Table 포함)
		if (key.material) {
			key.material->UpdateMeshShaderVariables(commandList);
		}

		// 3. 인스턴스 데이터 바인딩 (Slot 3번 - t100)
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = inst_cb->GetGPUVirtualAddress();
		gpuAddr += currentOffset * sizeof(ObjectCB);
		commandList->SetGraphicsRootShaderResourceView(4, gpuAddr);

		// 4. 인스턴싱 드로우 콜
		key.mesh->Render(commandList, count);

		currentOffset += count;
	}
	//batches.clear(); // 프레임 종료 후 초기화
}