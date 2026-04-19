#include "pch.h"
// Server쪽 Object
#include "Object.h"
#include "Component.h"


CObject::CObject(OBJECT_TYPE type)
	: obj_type(type)
	, obj_id(-1)
	, room_id(-1)
	, last_simulated_time(0.0f)
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

CObject::~CObject()
{

}

void CObject::Initialize()
{

}

void CObject::Update(const float elapsedTime)
{
	for (auto& component : components) {
		if (component->is_enable)
			component->Update(elapsedTime);
	}
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
	pitch = std::clamp(pitchDeg, -89.9f, 89.9f);

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(
		0.0f,
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