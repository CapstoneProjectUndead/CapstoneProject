#include "pch.h"
// Server쪽 Object
#include "Object.h"
#include "Player.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"


CObject::CObject()
	: obj_id(-1)
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

CObject::~CObject()
{

}

void CObject::Update(const float elapsedTime)
{

}

void CObject::SetComponent(std::shared_ptr<CComponent> component)
{
	component->owner = this;
	components.push_back(component);
	component->Initialize();
}

shared_ptr<CPlayer> CObject::CreatePlayer()
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();

	// Player 위치 지정 (임시)
	XMFLOAT3 pos{};
	pos.x = rand() % 3 - 2;
	pos.y = 0.1;
	pos.z = rand() % 3 - 2;
	player->SetPosition(pos);

	// ---------------------------------------------------
	// 서버 플레이어에게 충돌체(Collider) 달아주기
	// ---------------------------------------------------
	std::string fileName{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	// Mesh 로드 + totalBounds 계산
	BoundingBox totalBounds;
	bool firstBounds = true;

	for (const auto& child : frameRoot->childrens) {

		if (child->mesh.positions.empty())
			continue;

		// bounds merge
		if (firstBounds) {
			totalBounds = child->mesh.bounds;
			firstBounds = false;
		}
		else {
			BoundingBox::CreateMerged(totalBounds, totalBounds, child->mesh.bounds);
		}
	}

	// ColliderComponent 생성
	std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.x, totalBounds.Center);
	auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);
	player->SetComponent(collider);
	CPhysicsManager::GetInstance().SetCollider(collider);
	
	player->UpdateWorldMatrix();
	collider->Update(0.0f);

	return player;
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