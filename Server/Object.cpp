#include "pch.h"
// Server쪽 Object
#include "Object.h"
#include "Player.h"
#include "Collider.h"
#include "PhysicsManager.h"


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
	pos.x = rand() % 5 - 2;
	pos.y = 1;
	pos.z = rand() % 5 - 2;
	player->SetPosition(pos);

	// ---------------------------------------------------
	// 서버 플레이어에게 충돌체(Collider) 달아주기
	// ---------------------------------------------------
	// 1. 바운딩 박스(혹은 구)의 크기 설정 (클라이언트 캐릭터 크기와 비슷하게 세팅)
	BoundingBox playerBounds;
	playerBounds.Center = XMFLOAT3(0.0f, 0.5f, 0.0f); // 발밑이 0이라면 센터는 0.5 높이
	playerBounds.Extents = XMFLOAT3(0.5f, 0.5f, 0.5f); // 반지름 혹은 절반 크기

	// 2. Shape 생성 (클라이언트가 CSphereShape를 쓰므로 동일하게)
	// 인자: (반지름, 중심점)
	std::unique_ptr<CColliderShape> shape = std::make_unique<CSphereShape>(0.5f, playerBounds.Center);

	// 3. 콜라이더 컴포넌트 생성 및 장착
	auto collider = std::make_shared<CColliderComponent>(shape, playerBounds);
	collider->owner = player.get(); //
	player->SetComponent(collider);

	// 4. 물리 매니저에 등록
	CPhysicsManager::GetInstance().SetCollider(collider);
	// ---------------------------------------------------

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