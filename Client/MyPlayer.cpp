#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "KeyManager.h"
#include "ServerPacketHandler.h"

CMyPlayer::CMyPlayer()
	: CPlayer()
{
    is_my_player = true;
}

void CMyPlayer::Update(float elapsedTime)
{
	CPlayer::Update(elapsedTime);

	// 1. 입력 캡처 및 회전 (매 프레임)
	InputData currentInput;
	CaptureInput(currentInput);

	// 2. 회전 처리
	ProcessRotation();

	// 3. 이동 예측 (매 프레임 - 1000 FPS의 부드러움 유지)
	PredictMove(currentInput, elapsedTime);

	// 누적 DT
	dt_accumulator += elapsedTime;
	move_packet_send_timer -= elapsedTime;
	
	// 4. 서버 전송 (60hz, 드랍 프레임 대비 while)
	while (move_packet_send_timer <= 0) {

		move_packet_send_timer += move_packet_send_delay;

		float sendDt = min(dt_accumulator, move_packet_send_delay);

		C_Input inputPkt;
		inputPkt.seq_num = ++client_seq_counter;
		inputPkt.deltaTime = sendDt;

		inputPkt.info.id = obj_id;
		inputPkt.info.w = currentInput.w;
		inputPkt.info.a = currentInput.a;
		inputPkt.info.s = currentInput.s;
		inputPkt.info.d = currentInput.d;
		inputPkt.info.yaw = yaw;
		inputPkt.info.pitch = pitch;
		inputPkt.info.state = state;

		// 패킷 전송
		if (auto s = session.lock())
			s->DoSend(CServerPacketHandler::MakeSendBuffer<C_Input>(inputPkt));

		// 5. [중요] 장부 기록 위치 및 데이터 수정
		// elapsedTime 대신 '지난 패킷 이후 누적된 시간(dt_accumulator)'을 저장합니다.
		history_deque.push_back({
			inputPkt.seq_num,
			sendDt,   // 0.001이 아니라 약 0.0166이 들어감
			currentInput,
			position    // 현재 예측된 최신 위치
			});

		if (history_deque.size() > 600)
			history_deque.pop_front();

		// 누적 시간 초기화
		dt_accumulator -= sendDt;
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

				// 회전 적용
				SetYawPitch(yaw, pitch);
				UpdateWorldMatrix();
			}
			if (KEY_PRESSED(KEY::RBTN))
				Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}
	}
}

void CMyPlayer::ClientAuthorityMove(float elapsedTime)
{
	// 입력처리
	ProcessInput();

	// 상태 update
	if (direction.x == 0 && direction.z == 0)
		state = PLAYER_STATE::IDLE;
	else
		state = PLAYER_STATE::WALK;

	// 여기서 플레이어 위치와 방향 정보를 캐싱
	{
		// 1초에 5번씩 서버에 패킷을 보낸다.
		move_packet_send_timer -= elapsedTime;

		if (move_packet_send_timer <= 0) {

			move_packet_send_timer = move_packet_send_delay;

			C_Move movePkt;
			movePkt.info.id = obj_id;
			movePkt.info.x = position.x;
			movePkt.info.y = position.y;
			movePkt.info.z = position.z;
			movePkt.info.state = state;

			// 별도의 함수 호출 없이 멤버 변수(참조자)를 직접 사용
			// 1. Pitch: Look 벡터의 Y축 기울기
			movePkt.info.pitch = pitch;

			// 2. Yaw: Look 벡터의 X, Z 평면상의 방향
			movePkt.info.yaw = yaw;

			movePkt.info.roll = 0;

			SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Move>(movePkt);
			if (session.lock() != nullptr)
				session.lock()->DoSend(sendBuffer);
		}
	}
}

void CMyPlayer::ServerAuthorityMove(float elpasedTime)
{
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
	// 1. 서버가 확인한 입력까지 제거 
	while (!history_deque.empty() &&
		history_deque.front().seq_num <= last_seq)
	{
		history_deque.pop_front();
	}

	// 2. 서버 좌표로 스냅
	SetPosition(serverPos);

	// 3. 남은 미래 입력 재시뮬
	for (auto& frame : history_deque) {
		
		XMFLOAT3 replayDir{ 0.f, 0.f, 0.f };
		if (frame.input.w) replayDir.z++;
		if (frame.input.s) replayDir.z--;
		if (frame.input.a) replayDir.x--;
		if (frame.input.d) replayDir.x++;

		if (replayDir.x != 0 || replayDir.z != 0)
			Move(replayDir, frame.deltaTime);

		// 장부 위치 갱신
		frame.predictedPos = position;
	}
}