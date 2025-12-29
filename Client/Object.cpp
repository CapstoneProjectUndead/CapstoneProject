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
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 pos{ GetPosition()};
	XMFLOAT3 look{GetLook()};
	XMFLOAT3 right{ GetRight()};

	XMFLOAT3 shift = XMFLOAT3(0, 0, 0);
	if (direction.z > 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&look), distance)));
	}if (direction.z < 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&look), -distance)));
	}if (direction.x < 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&right), -distance)));
	}if (direction.x > 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&right), distance)));
	}
	Move(shift);
}

void CObject::Move(const XMFLOAT3 shift)
{
	XMFLOAT3 position{ GetPosition() };
	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&shift)));
	SetPosition(position);
}

void CObject::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	XMFLOAT4X4 TWorldMatrix;
	XMStoreFloat4x4(&TWorldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&world_matrix)));
	commandList->SetGraphicsRoot32BitConstants(0, 16, &TWorldMatrix, 0);
}

void CObject::Render(ID3D12GraphicsCommandList* commandList)
{
	for (const auto& mesh : meshes)
		mesh->Render(commandList);
}

void CObject::SetPosition(float x, float y, float z)
{
	world_matrix._41 = x;
	world_matrix._42 = y;
	world_matrix._43 = z;
}

void CObject::Animate(float elapsedTime, CCamera* camera)
{
	static float angle = 0.0f;
	angle += elapsedTime * 0.5f; // 천천히 회전
	
	
	XMMATRIX rotY = XMMatrixRotationY(angle);
	XMMATRIX trans = XMMatrixTranslation(world_matrix._41,
		world_matrix._42,
		world_matrix._43);

	XMMATRIX world = rotY * trans;
	XMStoreFloat4x4(&world_matrix, world);
}

// billboard
void CBillboardObject::Animate(float elapsedTime, CCamera* camera)
{
	XMFLOAT3 cameraPos = camera->GetPos();
	SetLookAt(cameraPos);
}

void CBillboardObject::SetLookAt(XMFLOAT3& target)
{
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 pos{ world_matrix._41, world_matrix._42, world_matrix._43};
	XMFLOAT3 look{ Vector3::Normalize(Vector3::Subtract(target, pos)) };
	XMFLOAT3 right{ Vector3::CrossProduct(look, up, true) };
	
	world_matrix._11 = right.x; world_matrix._12 = right.y; world_matrix._13 = right.z;
	world_matrix._21 = up.x; world_matrix._22 = up.y; world_matrix._23 = up.z;
	world_matrix._31 = look.x; world_matrix._32 = look.y; world_matrix._33 = look.z;
}
