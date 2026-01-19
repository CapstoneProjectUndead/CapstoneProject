#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "Camera.h"
#include "KeyManager.h"
#include "ServerPacketHandler.h"


CMyPlayer::CMyPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	: CPlayer(device, commandList)
{
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

	is_my_player = true;
}

CMyPlayer::~CMyPlayer()
{

}

void CMyPlayer::Update(float elapsedTime)
{
	// 입력처리
	ProcessInput();

	camera->Update(position, elapsedTime);
	camera->GenerateViewMatrix();

	// 여기서 플레이어 위치와 방향 정보를 캐싱
	{
		// 10프레임에 한번씩 서버에 패킷을 보낸다.
		move_packet_send_timer -= elapsedTime;

		if (move_packet_send_timer <= 0) {

			move_packet_send_timer = move_packet_send_dely;

			C_Move movePkt;
			movePkt.info.id = obj_id;
			movePkt.info.x = position.x;
			movePkt.info.y = position.y;
			movePkt.info.z = position.z;

			SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Move>(movePkt);
			if (session.lock() != nullptr)
				session.lock()->DoSend(sendBuffer);
		}
	}
}

void CMyPlayer::ProcessInput()
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