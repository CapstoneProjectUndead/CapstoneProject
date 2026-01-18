#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Timer.h"
#include "KeyManager.h"

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

	// 메쉬 설정
	std::shared_ptr<CMesh> mesh = std::make_shared<CCubeMesh>(device, commandList);
	SetMesh(mesh);
}

void CPlayer::Update(float elapsedTime)
{
	// 입력처리
	ProcessInput();

	//Move(velocity);

	camera->Update(position, elapsedTime);
	camera->GenerateViewMatrix();

	// 감속
	XMVECTOR xmvVelocity = XMLoadFloat3(&velocity);
	XMVECTOR xmvDeceleration = XMVector3Normalize(XMVectorScale(xmvVelocity, -1.0f));
	float length = XMVectorGetX(XMVector3Length(xmvVelocity));
	float deceleration = friction * elapsedTime;
	if (deceleration > length) deceleration = length;
	XMStoreFloat3(&velocity, XMVectorAdd(xmvVelocity, XMVectorScale(xmvDeceleration, deceleration)));

	// 여기서 플레이어 위치와 방향 정보를 캐싱
	{
		C_Move movePkt;
		movePkt.info.x = position.x;
		movePkt.info.y = position.y;
		movePkt.info.z = position.z;
	}
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	if (camera) camera->Move(shift);
}

void CPlayer::ProcessInput()
{
	XMFLOAT3 direction{};

	if (KEY_PRESSED(KEY::W)) direction.z++;
	if (KEY_PRESSED(KEY::S)) direction.z--;
	if (KEY_PRESSED(KEY::A)) direction.x--;
	if (KEY_PRESSED(KEY::D)) direction.x++;

	if (direction.x != 0 || direction.z != 0) {
		Move(direction, CTimer::GetInstance().GetTimeElapsed());
	}

	CKeyManager& keyManager{ CKeyManager::GetInstance() };

	if (KEY_PRESSED(KEY::LBTN) || KEY_PRESSED(KEY::RBTN)) {
		SetCursor(NULL);
		Vec2 prevMousePos{ keyManager.GetPrevMousePos() };
		Vec2 mouseDelta{ (keyManager.GetMousePos() - prevMousePos) / 3.0f };
		if (mouseDelta.x || mouseDelta.y)
		{
			if (KEY_PRESSED(KEY::LBTN))
				Rotate(mouseDelta.y, mouseDelta.x, 0.0f);
			if (KEY_PRESSED(KEY::RBTN))
				Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}
	}
}