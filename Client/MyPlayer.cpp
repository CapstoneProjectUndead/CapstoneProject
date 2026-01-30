#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "KeyManager.h"
#include "ServerPacketHandler.h"
#include "NetworkManager.h"

#undef min
#undef max

CMyPlayer::CMyPlayer()
	: CPlayer()
{
    is_my_player = true;
}

void CMyPlayer::Update(float elapsedTime)
{
	CPlayer::Update(elapsedTime);

	ServerAuthorityMove(elapsedTime);
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

				// 회전 적용
				SetYawPitch(yaw, pitch);
				UpdateWorldMatrix();
			}
			if (KEY_PRESSED(KEY::RBTN))
				Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}
	}
}

void CMyPlayer::ServerAuthorityMove(const float elapsedTime)
{
	// 1. 입력 캡처
	InputData currentInput;
	CaptureInput(currentInput);

	// 2. 회전
	ProcessRotation();

	// 3. 예측 이동
	PredictMove(currentInput, elapsedTime);

	// 서버 전송 타이머
	move_packet_send_timer -= elapsedTime;

	// 누적 시간
	dt_accumulator += elapsedTime;

	if (move_packet_send_timer <= 0.0f && IS_CONNECT)
	{
		move_packet_send_timer += move_packet_send_delay;

		// 패킷 생성
		C_Input inputPkt{};
		inputPkt.seq_num = ++client_seq_counter;

		inputPkt.info.id = obj_id;
		inputPkt.info.w = currentInput.w;
		inputPkt.info.a = currentInput.a;
		inputPkt.info.s = currentInput.s;
		inputPkt.info.d = currentInput.d;
		inputPkt.info.state = state;
		inputPkt.info.yaw = yaw;
		inputPkt.info.pitch = pitch;
		inputPkt.info.state = state;

		// 서버 전송
		if (auto s = session.lock())
			s->DoSend(CServerPacketHandler::MakeSendBuffer<C_Input>(inputPkt));

		// 4. 장부 기록
		history_deq.push_back({
			inputPkt.seq_num,
			dt_accumulator,
			currentInput,
			position
			});

		dt_accumulator = 0.0f;

		if (history_deq.size() > 600)
			history_deq.pop_front();
	}
}

void CMyPlayer::CaptureInput(InputData& currentInput)
{
	currentInput.w = KEY_PRESSED(KEY::W);
	currentInput.a = KEY_PRESSED(KEY::A);
	currentInput.s = KEY_PRESSED(KEY::S);
	currentInput.d = KEY_PRESSED(KEY::D);
}

void CMyPlayer::ProcessRotation()
{
	CKeyManager& keyManager = CKeyManager::GetInstance();
	if (KEY_PRESSED(KEY::LBTN)) {
		Vec2 mouseDelta = (keyManager.GetMousePos() - keyManager.GetPrevMousePos()) / 3.0f;
		if (mouseDelta.x || mouseDelta.y) {
			yaw += mouseDelta.x;
			pitch += mouseDelta.y;
			pitch = std::clamp(pitch, -89.9f, 89.9f);
			SetYawPitch(yaw, pitch);
			UpdateWorldMatrix();
		}
	}
}

void CMyPlayer::PredictMove(const InputData& input, float dt)
{
	XMFLOAT3 dir{ 0.f, 0.f, 0.f };
	if (input.w) dir.z++;
	if (input.s) dir.z--;
	if (input.a) dir.x--;
	if (input.d) dir.x++;

	// 상태 update
	if (dir.x == 0 && dir.z == 0)
		state = PLAYER_STATE::IDLE;
	else
		state = PLAYER_STATE::WALK;

	if (dir.x != 0 || dir.z != 0) {
		Move(dir, dt);
	}
}

void CMyPlayer::ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos)
{
	// 1. 서버 좌표로 스냅
	SetPosition(serverPos);

	// 2. 서버가 확인한 입력까지 제거 
	while (!history_deq.empty() &&
		history_deq.front().seq_num <= last_seq)
	{
		history_deq.pop_front();
	}

	// 3. 남은 미래 입력 재시뮬
	for (auto& frame : history_deq) {
		
		PredictMove(frame.input, frame.duration);

		// 장부 위치 갱신
		frame.predictedPos = position;
	}
}