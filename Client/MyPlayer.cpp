#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "KeyManager.h"
#include "ServerPacketHandler.h"
#include "NetworkManager.h"
#include "Movement.h"
#include "NetworkClockManager.h"
#include "User.h"
#include "Collider.h"
#include "PhysicsManager.h"

#undef min
#undef max

CMyPlayer::CMyPlayer()
	: CPlayer()
{
    is_my_player = true;
}

void CMyPlayer::Update(float elapsedTime)
{
	// 1초 주기로 서버와 Ping-Pong
	// 서버와 시간대를 맞추기 위한 작업
	SendPingToServer(elapsedTime);

	PreUpdate(elapsedTime);

	CPlayer::Update(elapsedTime);
}

void CMyPlayer::PreUpdate(float elapsedTime)
{
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
		if (auto move = GetComponent<CMovementComponent>())
			move->Move(direction, CTimer::GetInstance().GetTimeElapsed());
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
	CaptureInput(current_input);

	// 2. 회전
	ProcessRotation();

	// 3. 예측 이동
	PredictMove(current_input, elapsedTime);
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
			pitch = std::clamp(pitch, -89.9f, 60.0f);
			SetYawPitch(yaw, 0.0f);
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
		if (auto move = GetComponent<CMovementComponent>())
			move->Move(dir, dt);
	}
}

void CMyPlayer::RecordClientFrameHistory(const ClientFrameHistory& history)
{
	client_history_deq.push_back(history);

	if (client_history_deq.size() > CLIENT_HISTORY_MAX_SIZE)
		client_history_deq.pop_front();
}

void CMyPlayer::BeginSendInputPacket(float elapsedTime)
{
	// 서버 전송 타이머
	move_packet_send_timer -= elapsedTime;

	// 누적 시간 (이동 거리 누적)
	dt_accumulator += elapsedTime;

	if (move_packet_send_timer <= 0.0f && IS_CONNECT) {

		move_packet_send_timer += move_packet_send_delay;

		// 패킷 생성 & 서버 전송
		C_Input inputPkt{};
		SendInputPacket(inputPkt, current_input);

		// 장부 기록
		ClientFrameHistory history{};
		history.seq_num = inputPkt.seq_num;
		history.input = current_input;
		history.duration = dt_accumulator;
		history.predicted_pos = position;
		history.state = state;
		RecordClientFrameHistory(history);

		dt_accumulator = 0.0f;
	}
}

void CMyPlayer::SendInputPacket(C_Input& inputPkt, const InputData& input)
{
	inputPkt.seq_num = ++client_seq_counter;
	inputPkt.info.player_id = obj_id;
	inputPkt.room_id = room_id;
	inputPkt.info.w = input.w;
	inputPkt.info.a = input.a;
	inputPkt.info.s = input.s;
	inputPkt.info.d = input.d;
	inputPkt.info.yaw = yaw;
	inputPkt.info.pitch = pitch;
	inputPkt.info.state = state;
	inputPkt.scene_type = current_scene_type;

	// 서버 전송
	if (auto s = session.lock())
		s->DoSend(CServerPacketHandler::MakeSendBuffer<C_Input>(inputPkt));
}

void CMyPlayer::SendPingToServer(const float elapsedTime)
{
	dt_ping_accumulator += elapsedTime;

	if (dt_ping_accumulator >= 1.0f && IS_CONNECT) {
		if (GetSession()) {
			CNetworkClockManager::GetInstance().SendPing(GetSession());
			dt_ping_accumulator -= 1.0f;
		}
	}
}

void CMyPlayer::SimulateMove(const InputData& input, float dt)
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

	// 움직임 시뮬레이션 (속도 계산 + 물리 이동)
	auto move = GetComponent<CMovementComponent>();
	auto collider = GetComponent<CColliderComponent>();

	if (move) {
		// A. 속도 계산 (Velocity 갱신)
		move->Simulate(dir, dt);
	}

	if (collider) {
		// B. 물리 적용 (충돌 처리 + Slide + 실제 Position 변경)
		CPhysicsManager::GetInstance().SimulateSingleObject(collider, dt);
	}
}

void CMyPlayer::ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos)
{
	// 1. 서버 좌표와 현재 내 좌표의 거리 계산
	XMFLOAT3 diff = Vector3::Subtract(serverPos, position);
	float errorDist = Vector3::Length(diff);

	// 2. 보정 정책
	if (errorDist < 0.01f) {
		//position = Vector3::Add(position, diff, 0.1f);
		return;
	}
	else if (errorDist < 0.2f) {
		position = Vector3::Add(position, diff, 0.5f);
		return;
	}

	// 3. 서버 좌표로 스냅
	SetPosition(serverPos);

	// 4. 서버가 확인한 입력까지 제거 
	while (!client_history_deq.empty() &&
		client_history_deq.front().seq_num <= last_seq) {
		client_history_deq.pop_front();
	}

	// 5. 남은 미래 입력 재시뮬
	for (auto& frame : client_history_deq) {
		
		SimulateMove(frame.input, frame.duration);

		// 장부 위치 갱신
		frame.predicted_pos = position;
	}
}