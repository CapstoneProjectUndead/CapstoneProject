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

void CObject::Move(const XMFLOAT3& direction, float elapsedTime)
{
	// 1. 입력 → 가속도
	XMFLOAT3 accel{};

	if (direction.z > 0) accel = Vector3::Add(accel, look);
	if (direction.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(look, -1));
	if (direction.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(right, -1));
	if (direction.x > 0) accel = Vector3::Add(accel, right);

	// 2. 가속 적용
	velocity = Vector3::Add(velocity, Vector3::ScalarProduct(accel, speed * elapsedTime));

	// 3. 최대 속도 제한
	float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (lenXZ > max_speed) {
		float ratio = max_speed / lenXZ;
		velocity.x *= ratio;
		velocity.z *= ratio;
	}

	// 4. 이동
	position = Vector3::Add(position, Vector3::ScalarProduct(velocity, elapsedTime));

	// 5. 감속 (마찰)
	float speedLen = Vector3::Length(velocity);
	float decel = friction * elapsedTime;
	if (decel > speedLen) decel = speedLen;

	velocity = Vector3::Add(velocity, Vector3::ScalarProduct(velocity, -decel, true));
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