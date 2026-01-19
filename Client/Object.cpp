#include "stdafx.h"
#include "Shader.h"
#include "Object.h"
#include "Camera.h"

CObject::CObject()
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

void CObject::ReleaseUploadBuffer()
{
	// 정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	for(const auto& mesh : meshes)
		mesh->ReleaseUploadBuffer();
}

void CObject::SetMesh(std::shared_ptr<CMesh>& otherMesh)
{
	meshes.push_back(otherMesh);
}

void CObject::SetTexture(CTexture* otherTexture)
{
	texture.reset(otherTexture);
}

void CObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	world_matrix = Matrix4x4::Multiply(rotateMatrix, world_matrix);
}

void CObject::Move(const XMFLOAT3 direction, float distance)
{
	XMFLOAT3 shift{};
	if (direction.z > 0) {
		shift = Vector3::Add(shift, Vector3::ScalarProduct(look, distance));
	}if (direction.z < 0) {
		shift = Vector3::Add(shift, Vector3::ScalarProduct(look, -distance));
	}if (direction.x < 0) {
		shift = Vector3::Add(shift, Vector3::ScalarProduct(right, -distance));
	}if (direction.x > 0) {
		shift = Vector3::Add(shift, Vector3::ScalarProduct(right, distance));
	}
	Move(Vector3::ScalarProduct(shift, speed, false));
}

void CObject::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
}

void CObject::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	{
		ObjectCB cb{};
		XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world_matrix));
		XMStoreFloat4x4(&cb.world_matrix, worldT);

		UINT8* mapped = nullptr;
		object_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		memcpy(mapped, &cb, sizeof(cb));
		object_cb->Unmap(0, nullptr);

		commandList->SetGraphicsRootConstantBufferView(0, object_cb->GetGPUVirtualAddress());
	}

	{
		MaterialCB cb{material.albedo};

		UINT8* mapped = nullptr;
		material_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		memcpy(mapped, &cb, sizeof(cb));
		material_cb->Unmap(0, nullptr);

		commandList->SetGraphicsRootConstantBufferView(2, material_cb->GetGPUVirtualAddress());
	}
}

void CObject::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		ObjectCB cb{};
		object_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<ObjectCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	}

	{
		MaterialCB cb{};
		material_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<MaterialCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	}
}

void CObject::Render(ID3D12GraphicsCommandList* commandList)
{
	for (const auto& mesh : meshes)
		mesh->Render(commandList);
}

void CObject::Animate(float elapsedTime, CCamera* camera)
{
	static float angle = 0.0f;
	angle += elapsedTime * 0.5f; // 천천히 회전
	
	XMMATRIX rotY = XMMatrixRotationY(angle);
	XMMATRIX trans = XMMatrixTranslation(world_matrix._41, world_matrix._42, world_matrix._43);

	XMMATRIX world = rotY * trans;
	XMStoreFloat4x4(&world_matrix, world);
}
