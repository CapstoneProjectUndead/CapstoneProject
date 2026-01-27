#include "pch.h"
// Server쪽 Object
#include "Object.h"
#include "Player.h"

atomic<uint64> CObject::s_idGenerator = 1;

CObject::CObject()
	: obj_id(-1)
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

CObject::~CObject()
{

}

shared_ptr<CPlayer> CObject::CreatePlayer()
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	player->SetID(s_idGenerator);
	++s_idGenerator;

	return player;
}

void CObject::Update(const float elapsedTime)
{

}

void CObject::Move(const XMFLOAT3& direction, float distance)
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

void CObject::Move(const XMFLOAT3& shift)
{
	position = Vector3::Add(position, shift);
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