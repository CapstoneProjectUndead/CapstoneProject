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
	ProcessRotation();

	// 2. 이동 예측 (매 프레임 - 1000 FPS의 부드러움 유지)
	PredictMove(currentInput, elapsedTime);

	// 이번 프레임의 시간을 누적시킨다.
	dt_accumulator += elapsedTime;

	// 3. 서버 전송 주기 (60Hz)
	move_packet_send_timer -= elapsedTime;
	if (move_packet_send_timer <= 0)
	{
		move_packet_send_timer += move_packet_send_delay;

		C_Input inputPkt;
		inputPkt.seq_num = ++client_seq_counter;
		inputPkt.info.id = obj_id;
		inputPkt.info.input = currentInput;
		inputPkt.info.state = state;
		inputPkt.info.yaw = yaw;
		inputPkt.info.pitch = pitch;

		// 패킷 전송
		if (auto s = session.lock())
			s->DoSend(CServerPacketHandler::MakeSendBuffer<C_Input>(inputPkt));

		// 4. [중요] 장부 기록 위치 및 데이터 수정
		// elapsedTime 대신 '지난 패킷 이후 누적된 시간(dt_accumulator)'을 저장합니다.
		history_deque.push_back({
			inputPkt.seq_num,
			dt_accumulator,   // 0.001이 아니라 약 0.0166이 들어감
			currentInput,
			position    // 현재 예측된 최신 위치
			});

		// 누적 시간 초기화
		dt_accumulator = 0.0f;

		if (history_deque.size() > 600)
			history_deque.pop_front();
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
	// 1. 장부에서 서버가 확인해준 시퀀스(last_seq) 이전 기록 삭제
	while (!history_deque.empty() && history_deque.front().seq_num < last_seq) {
		history_deque.pop_front();
	}

	if (!history_deque.empty() && history_deque.front().seq_num == last_seq)
	{
		// 2. 서버가 계산한 좌표와 내가 당시 예측했던 좌표 비교
		float error = Vector3::Distance(history_deque.front().predictedPos, serverPos);

		// 서버 확인이 끝난 데이터는 삭제
		history_deque.pop_front();

		// 3. 오차가 임계치(0.05f)보다 크면 보정 시작
		if (error > 0.05f)
		{
			// [A] 서버 위치로 즉시 강제 이동 (Snap) (서버가 맞다)
			SetPosition(serverPos);

			// [B] 장부에 남은 "아직 서버 확인을 못 받은 미래 입력들"을 재시뮬레이션
			for (auto& frame : history_deque)
			{
				// InputData를 방향 벡터로 변환 (PredictMove와 동일한 로직)
				XMFLOAT3 replayDir{ 0.f, 0.f, 0.f };
				if (frame.input.w) replayDir.z++;
				if (frame.input.s) replayDir.z--;
				if (frame.input.a) replayDir.x--;
				if (frame.input.d) replayDir.x++;

				// 재시뮬레이션 수행
				if (replayDir.x != 0 || replayDir.z != 0) {
					Move(replayDir, frame.deltaTime);
				}

				// [중요] 재계산된 위치로 장부의 predictedPos를 업데이트해줍니다.
				// 그래야 다음 서버 패킷이 왔을 때 또 틀렸다고 판단하지 않습니다.
				frame.predictedPos = position;
			}
		}
	}
}