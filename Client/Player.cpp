#include "stdafx.h"
#include "Player.h"
#include "Camera.h"

extern HWND ghWnd;

// Player
CPlayer::CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	is_visible = true;

	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	camera = std::make_shared<CCamera>();
	camera->SetViewport(0, 0, width, height);
	camera->SetScissorRect(0, 0, width, height);
	camera->GenerateProjectionMatrix(1.0f, 500.0f, (float)width / (float)height, 90.0f);
	camera->SetLookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	camera->SetCameraOffset(XMFLOAT3(0.0f, 2.0f, -5.0f)); 
	camera->SetPlayer(this);

	// 메쉬 설정
	std::shared_ptr<CMesh> mesh = std::make_shared<CCubeMesh>(device, commandList);
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	SetMesh(mesh);
}

void CPlayer::Update(float elapsedTime)
{
	Move(velocity);

	camera->Update(position, elapsedTime);
	camera->GenerateViewMatrix();

	// 감속
	XMVECTOR xmvVelocity = XMLoadFloat3(&velocity);
	XMVECTOR xmvDeceleration = XMVector3Normalize(XMVectorScale(xmvVelocity, -1.0f));
	float length = XMVectorGetX(XMVector3Length(xmvVelocity));
	float deceleration = friction * elapsedTime;
	if (deceleration > length) deceleration = length;
	XMStoreFloat3(&velocity, XMVectorAdd(xmvVelocity, XMVectorScale(xmvDeceleration, deceleration)));

	OnUpdateTransform();
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	if (camera) camera->Move(shift);
}

void CPlayer::Move(const XMFLOAT3 direction, float distance)
{
	CObject::Move(direction, distance);
}

void CPlayer::OnUpdateTransform()
{
	world_matrix._11 = right.x; world_matrix._12 = right.y; world_matrix._13 = right.z;
	world_matrix._21 = up.x; world_matrix._22 = up.y; world_matrix._23 = up.z;
	world_matrix._31 = look.x; world_matrix._32 = look.y; world_matrix._33 = look.z;
	world_matrix._41 = position.x; world_matrix._42 = position.y; world_matrix._43 = position.z;
}
