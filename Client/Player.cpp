#include "stdafx.h"
#include "Player.h"
#include "Camera.h"

extern HWND ghWnd;

// Player
CPlayer::CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));

	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	camera = std::make_shared<CCamera>();
	camera->SetViewport(0, 0, width, height);
	camera->SetScissorRect(0, 0, width, height);
	camera->GenerateProjectionMatrix(1.0f, 500.0f, (float)width / (float)height, 90.0f);
	camera->SetCameraOffset(XMFLOAT3(0.0f, 2.0f, -5.0f)); 
	camera->SetPlayer(this);
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
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	if (camera) camera->Move(shift);
}