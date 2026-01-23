#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "KeyManager.h"
#include "ServerPacketHandler.h"

CMyPlayer::CMyPlayer()
	: CPlayer()
{
	// ī�޶� ��ü ����
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

	// ���� update
	if (direction.x == 0 && direction.z == 0)
		state = PLAYER_STATE::IDLE;
	else
		state = PLAYER_STATE::WALK;

	camera->Update(position, elapsedTime);
	camera->GenerateViewMatrix();

	// ���⼭ �÷��̾� ��ġ�� ���� ������ ĳ��
	// 여기서 플레이어 위치와 방향 정보를 캐싱
	{
		// 1�ʿ� 5���� ������ ��Ŷ�� ������.
		move_packet_send_timer -= elapsedTime;

		if (move_packet_send_timer <= 0) {

			move_packet_send_timer = move_packet_send_dely;

			C_Move movePkt;
			movePkt.info.id = obj_id;
			movePkt.info.x = position.x;
			movePkt.info.y = position.y;
			movePkt.info.z = position.z;
			movePkt.info.state = state;

			// ������ �Լ� ȣ�� ���� ��� ����(������)�� ���� ���
			// 1. Pitch: Look ������ Y�� ����
			movePkt.info.pitch = pitch;

			// 2. Yaw: Look ������ X, Z ������ ����
			movePkt.info.yaw = yaw;

			movePkt.info.roll = 0;

			SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Move>(movePkt);
			if (session.lock() != nullptr)
				session.lock()->DoSend(sendBuffer);
		}
	}
}

void CMyPlayer::ProcessInput()
{
	direction = XMFLOAT3{0.f, 0.f, 0.f};

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
			if (KEY_PRESSED(KEY::LBTN)) {
				yaw += mouseDelta.x;
				pitch += mouseDelta.y;
				pitch = std::clamp(pitch, -89.9f, 89.9f);

				// ȸ�� ����
				SetYawPitch(yaw, pitch);
				UpdateWorldMatrix();
			}
			if (KEY_PRESSED(KEY::RBTN))
				Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}
	}
}