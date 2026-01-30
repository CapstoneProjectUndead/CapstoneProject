#include "stdafx.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Camera.h"
#include "Object.h"

CObject::CObject()
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

ID3D12Resource* CObject::GetTextureResource() const
{
	return texture->GetTextureResource();
}
void CObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	world_matrix = Matrix4x4::Multiply(rotateMatrix, world_matrix);
}

void CObject::Move(const XMFLOAT3 direction, float deltaTime)
{
	XMFLOAT3 accel{};

	if (direction.z > 0) accel = Vector3::Add(accel, look);
	if (direction.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(look, -1));
	if (direction.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(right, -1));
	if (direction.x > 0) accel = Vector3::Add(accel, right);

	// 가속도 적용: velocity += accel * speed * deltaTime
	velocity = Vector3::Add(velocity, Vector3::ScalarProduct(accel, speed * deltaTime));
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
		MaterialCB cb{};
		memcpy(&cb, &material, sizeof(Material));

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

void CObject::Update(float elapsedTime)
{
	// 중력 적용
	//velocity = Vector3::Add(velocity, Vector3::ScalarProduct(gravity, deltaTime));

	// 최대 속도 제한
	float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (lenXZ > max_speed) {
		float ratio = max_speed / lenXZ;
		velocity.x *= ratio;
		velocity.z *= ratio;
	}

	// 이동
	position = Vector3::Add(position, Vector3::ScalarProduct(velocity, elapsedTime));

	// 감속(마찰)
	float speedLen = Vector3::Length(velocity);
	float decel = friction * elapsedTime;
	if (decel > speedLen) decel = speedLen;

	velocity = Vector3::Add(velocity, Vector3::ScalarProduct(velocity, -decel, true));
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
