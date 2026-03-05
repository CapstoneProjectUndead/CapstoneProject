#include "stdafx.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Object.h"
#include "MeshRenderer.h"
#include "Material.h"

CObject::CObject(OBJECT_TYPE type)
	: obj_type(type)
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

void CObject::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CreateConstantBuffers(device, commandList);

}

void CObject::ReleaseUploadBuffer()
{
	// 정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	for (auto& component : components)
		if(component->is_enable)
			component->ReleaseUploadBuffer();
}

void CObject::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	{
		XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world_matrix));
		XMStoreFloat4x4(&mapped->world_matrix, worldT);

		commandList->SetGraphicsRootConstantBufferView(0, object_cb->GetGPUVirtualAddress());
	}

	for (auto& component : components) {
		if (component->is_enable)
			component->UpdateShaderVariables(commandList);
	}
}

void CObject::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		object_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<ObjectCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		object_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	}

	for (auto& component : components) {
		component->CreateConstantBuffers(device, commandList);
	}
}

void CObject::Render(ID3D12GraphicsCommandList* commandList)
{
	auto meshRenderer = GetComponents<CMeshRendererComponent>();

	// mesh, collider(for debugging), material render
	for (auto& renderer : meshRenderer)
		if (renderer->is_enable)
			renderer->Render(commandList);
}

void CObject::SetComponent(std::shared_ptr<CComponent> component)
{
	component->owner = this;
	components.push_back(component);
	component->Initialize();
}

void CObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	world_matrix = Matrix4x4::Multiply(rotateMatrix, world_matrix);
}

void CObject::SetYaw(float _yaw)
{
	yaw = _yaw;
	UpdateLookRightFromYaw();
}

void CObject::SetYawPitch(float yawDeg, float pitchDeg)
{
	// pitch 제한 (이거 중요)
	pitchDeg = std::clamp(pitchDeg, -89.9f, 89.9f);

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(pitchDeg),
		XMConvertToRadians(yawDeg),
		0.0f  
	);

	XMStoreFloat4(&orientation, q);
}

void CObject::UpdateWorldMatrix()
{
	XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&orientation));
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&world_matrix, rot * trans);
}

void CObject::UpdateLookRightFromYaw()
{
	float rad = XMConvertToRadians(yaw);

	look.x = sinf(rad);
	look.y = 0.0f;
	look.z = cosf(rad);

	look = Vector3::Normalize(look);

	// Y-up 기준 Right 벡터
	right = XMFLOAT3(
		look.z,
		0.0f,
		-look.x
	);
}

void CObject::Update(const float elapsedTime)
{
	for (auto& component : components) {
		if (component->is_enable)
			component->Update(elapsedTime);
	}
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
