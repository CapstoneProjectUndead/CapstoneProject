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
	// 1. 이번 프레임의 입력 상태를 캡처
	InputData currentInput;
	currentInput.w = KEY_PRESSED(KEY::W);
	currentInput.a = KEY_PRESSED(KEY::A);
	currentInput.s = KEY_PRESSED(KEY::S);
	currentInput.d = KEY_PRESSED(KEY::D);

	// 2. 회전 즉시 적용 및 입력 데이터에 포함
	ProcessRotation();
	currentInput.pitch = this->pitch;
	currentInput.yaw = this->yaw;

	// 3. 이동 예측 (현재 좌표가 변경됨)
	PredictMove(currentInput, elapsedTime);

	// 4. 서버에 보낼 패킷 생성 및 전송
	C_Input inputPkt;
	inputPkt.seq_num = ++client_seq_counter;
	inputPkt.input = currentInput;

	SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Input>(inputPkt);
	if (auto s = session.lock())
		s->DoSend(sendBuffer);

	// 5. [중요] 장부에 기록 
	// 여기서 position은 PredictMove()가 끝난 후의 "예측된 새로운 좌표"입니다.
	history_deque.push_back({
		inputPkt.seq_num,
		elapsedTime,
		currentInput,
		this->position
		});

	// 장부가 너무 무한정 커지지 않게 관리 (예: 최근 600프레임 = 약 10초)
	if (history_deque.size() > 600)
		history_deque.pop_front();
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

			move_packet_send_timer = move_packet_send_dely;

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

	if (dir.x != 0 || dir.z != 0) {
		// 일단 서버 응답 없어도 내가 먼저 움직인다 (예측)
		Move(dir, dt);
	}
}

void CMyPlayer::ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos)
{
	// 1. 장부에서 서버가 확인해준 시퀀스(last_seq)를 찾습니다.
	while (!history_deque.empty() && history_deque.front().seq_num < last_seq) {
		history_deque.pop_front();
	}

	if (!history_deque.empty() && history_deque.front().seq_num == last_seq)
	{
		// [핵심] 같은 시퀀스 번호끼리 비교합니다!
		float error = Vector3::Distance(history_deque.front().predictedPos, serverPos);

		// 이미 서버가 확인해준 기록은 이제 쓸모없으니 지웁니다.
		history_deque.pop_front();

		// 2. 오차가 크면 (예: 서버는 벽에 막혔는데 나는 통과했다고 예측한 경우)
		if (error > 0.05f) // 임계치는 테스트하며 조절
		{
			// A. 서버 좌표로 강제 스냅 (과거로 되돌림)
			SetPosition(serverPos);

			// B. 남은 미래의 입력들을 그 자리에서 재시뮬레이션
			for (const auto& frame : history_deque) {
				//Move(frame.input, frame.deltaTime);
				
				// (선택 사항) 재시뮬레이션 후 예측 좌표를 갱신해두면 다음 보정 때 더 정확합니다.
				// frame.predictedPos = this->position; 
			}
		}
	}
}