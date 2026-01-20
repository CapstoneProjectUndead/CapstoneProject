#include "pch.h"
// ServerÂÊ Object
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

    Move(shift);
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

void CObject::Update(float elapsedTime)
{

}
