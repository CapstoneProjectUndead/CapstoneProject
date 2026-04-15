#include "stdafx.h"
#include "MyPlayer.h"
#include "KeyManager.h"
#include "ImGuiManager.h"

#include "ServerPacketHandler.h"
#include "NetworkManager.h"
#include "NetworkClockManager.h"
#include "User.h"

#include "Inventory.h"
#include "ItemFinder.h"
#include "QuickSlot.h"

#include "Movement.h"
#include "Animator.h"

#undef min
#undef max

CMyPlayer::CMyPlayer()
	: CPlayer()
	, gold(0)
	, is_ready(false)
	, current_input{ false, false, false, false, false, false }
{
    is_my_player = true;
}

void CMyPlayer::Update(float elapsedTime)
{
	// 1초 주기로 서버와 Ping-Pong
	// 서버와 시간대를 맞추기 위한 작업
	SendPingToServer(elapsedTime);

	PreUpdate(elapsedTime);

	// 멀티 플레이일 경우에만, 아래 로직이 실행
	if (!g_is_single && current_scene_type != SCENE_TYPE::CUSTOMS) {
		InterpolateMyPlayer(elapsedTime);
	}

	// 스테미나 Update
	if (g_is_single) {
		UpdateStamina(elapsedTime);
	}

	// "E" 키를 누르면 인벤토리를 열고/닫기
	if (current_scene_type == SCENE_TYPE::GAME && KEY_TAP(KEY::E)) {
		CKeyManager::GetInstance().SetMouseMode(!CKeyManager::GetInstance().GetMouseMode());
		inventory->ToggleOpen();
	}

	if (KEY_TAP(KEY::F)) {
		auto itemFinder = GetComponent<CItemFinder>();
		if (itemFinder) {
			itemFinder->Toggle();
			// 애니메이션 호출
			if (itemFinder->is_enable) {
				auto animator = GetComponent<CAnimatorComponent>();
				if (animator)
					animator->PlayAction("Ganga_search");
			}
			else {
				auto animator = GetComponent<CAnimatorComponent>();
				if (animator)
					animator->PlayAction("");
			}
		}
	}

	CPlayer::Update(elapsedTime);
}

void CMyPlayer::PreUpdate(float elapsedTime)
{
	ServerAuthorityMove(elapsedTime);
}

void CMyPlayer::OnCollect(IRenderer* renderer)
{
	CObject::OnCollect(renderer);

	auto animator = GetComponent<CAnimatorComponent>();
	if (animator) {
		// 싱글 모드일 때는 바로 장착
		if (g_is_single) {
			animator->RenderSocketModel(CAnimatorComponent::HAND_R, quick_slot->GetSelectedItemId());
		}
		else {
			// 멀티 모드에서는 서버의 허락을 받는다.
			if (equipped_item_id != 0)
				animator->RenderSocketModel(CAnimatorComponent::HAND_R, equipped_item_id);
		}

		auto itemFinder = GetComponent<CItemFinder>();
		if (itemFinder && itemFinder->is_enable) {
			animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_R, NULL, "dowsing_rod_0307");
			animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_L, NULL, "dowsing_rod_0307");
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

void CMyPlayer::InterpolateMyPlayer(float elapsedTime)
{
	// 목적지 좌표 세팅
	XMFLOAT3 destPos = { dest_info.x, dest_info.y, dest_info.z };

	// 현재 위치와 목적지의 평면(XZ) 거리 차이 계산
	float dx = destPos.x - position.x;
	float dz = destPos.z - position.z;
	float distSq = (dx * dx) + (dz * dz);

	// 임계값을 10cm (0.1f * 0.1f)로 살짝 넉넉하게 키움!
	float thresholdSq = 0.01f;

	// 거리에 따른 기본 상태 결정
	if (distSq > thresholdSq) {
		if (current_input.shift && !stamina_exhausted)
			state = PLAYER_STATE::RUN;
		else
			state = PLAYER_STATE::WALK;
	}
	else {
		state = PLAYER_STATE::IDLE;
	}

	if (std::abs(server_velocity.y) > 0.2f) {
		state = PLAYER_STATE::WALK; // 나중에 JUMP로 바꿀 곳
	}

	// 부드러운 이동 (Lerp)
	float lerpSpeed = 8.0f * elapsedTime;
	position = Vector3::Lerp(position, destPos, lerpSpeed);
}

void CMyPlayer::ServerAuthorityMove(const float elapsedTime)
{
	// 1. 입력 캡처
	CaptureInput(current_input);

	// 2. 회전
	ProcessRotation();

	// 3. 예측 이동
	if (g_is_single) {
		PredictMove(current_input, elapsedTime);
	}
}

void CMyPlayer::CaptureInput(InputData& currentInput)
{
	currentInput.w = KEY_PRESSED(KEY::W);
	currentInput.a = KEY_PRESSED(KEY::A);
	currentInput.s = KEY_PRESSED(KEY::S);
	currentInput.d = KEY_PRESSED(KEY::D);
	currentInput.space = KEY_PRESSED(KEY::SPACE);
	currentInput.shift = KEY_PRESSED(KEY::LSHIFT);
}

void CMyPlayer::ProcessRotation()
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	CKeyManager& keyManager = CKeyManager::GetInstance();
	bool isGame = keyManager.GetMouseMode();
	if (KEY_PRESSED(KEY::LBTN) || isGame) {
		Vec2 mouseDelta;
		if(isGame)
			mouseDelta = keyManager.GetMouseDrag() / 5.0f;
		else
			mouseDelta = (keyManager.GetMousePos() - keyManager.GetPrevMousePos()) / 3.0f;
		if (mouseDelta.x || mouseDelta.y) {
			yaw += mouseDelta.x;
			pitch += mouseDelta.y;
			pitch = std::clamp(pitch, -89.9f, 89.9f);
			SetYawPitch(yaw, 0.0f);
			UpdateWorldMatrix();
		}
	}
}


void CMyPlayer::PredictMove(const InputData& input, float dt)
{
	auto move = GetComponent<CMovementComponent>();
	if (!move || current_scene_type == SCENE_TYPE::CUSTOMS) return;
	
	// 키 처리
	XMFLOAT3 dir{ 0.f, 0.f, 0.f };
	if (input.w) dir.z++;
	if (input.s) dir.z--;
	if (input.a) dir.x--;
	if (input.d) dir.x++;
	if (input.space) {
		move->Jump();
	}

	// 상태 update
	bool isMoving = (dir.x != 0 || dir.z != 0);

	// 움찔거리는 거 방지
	grounded_timer = is_grounded ? 0.1f : (grounded_timer - dt);

	if (grounded_timer > 0.0f) {
		if (!isMoving) {
			state = PLAYER_STATE::IDLE;
		}
		else {
			if (input.shift && !stamina_exhausted) {
				state = PLAYER_STATE::RUN;
				move->Run();
			}
			else {
				state = PLAYER_STATE::WALK;
				move->UnRun();
			}
		}
	}
	else {
		state = PLAYER_STATE::JUMP; // 확실히 공중일 때만 점프 상태
	}

	if (dir.x != 0 || dir.z != 0) {
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
		history.predicted_velocity = velocity;
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
	inputPkt.info.space = input.space;
	inputPkt.info.shift = input.shift;
		
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

void CMyPlayer::SimulateMove(const InputData& input, float elapsedTime)
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

	if (move) {
		// A. 속도 계산 (Velocity 갱신)
		move->Simulate(dir, elapsedTime);
	}
}

void CMyPlayer::UpdateStamina(float elapsedTime)
{
	const float drainPerSec    = 100.0f; // 뛸 때 초당 감소 (10초면 바닥)
	const float regenPerSec    =  50.0f; // 쉴 때 초당 회복 (20초면 풀충전)
	const float recoverThreshold = 200.0f; // 이 값 이상 회복돼야 다시 달리기 허용

	if (state == PLAYER_STATE::RUN) {

		accumulate_stamina -= drainPerSec * elapsedTime;

		if (accumulate_stamina <= 0.0f) {

			accumulate_stamina = 0.0f;
			stamina_exhausted = true;

			if (auto move = GetComponent<CMovementComponent>()) {

				move->UnRun();

				// 이번 프레임 velocity도 즉시 walk 속도로 재조정
				float ws = move->GetWalkSpeed();
				float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

				if (lenXZ > ws && lenXZ > 0.0001f) {
					float ratio = ws / lenXZ;
					velocity.x *= ratio;
					velocity.z *= ratio;
				}
			}
		}
	}
	else {
		accumulate_stamina += regenPerSec * elapsedTime;

		if (accumulate_stamina > static_cast<float>(stat.maxStamina))
			accumulate_stamina = static_cast<float>(stat.maxStamina);

		// 일정량 이상 회복돼야 달리기 재허용 (0→1 플리커 방지)
		if (stamina_exhausted && accumulate_stamina >= recoverThreshold)
			stamina_exhausted = false;
	}

	// uint32인 stat.stamina에 동기화 (UI 표시용)
	stat.stamina = static_cast<uint32>(accumulate_stamina);
}

void CMyPlayer::ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos)
{
	// 1. 장부(History)에서 서버가 말한 그 당시의 내 기록 찾기
	auto it = std::find_if(client_history_deq.begin(), client_history_deq.end(),
		[last_seq](const ClientFrameHistory& h) { return h.seq_num == last_seq; });
	
	if (it == client_history_deq.end())
		return;
	
	// 2. 과거의 예측 위치와 서버의 진짜 위치 비교
	XMFLOAT3 error = Vector3::Subtract(serverPos, it->predicted_pos);
	float errorDist = Vector3::Length(error);
	
	// 3. 사용된 과거 기록은 삭제 (메모리 누수 방지)
	while (!client_history_deq.empty() && client_history_deq.front().seq_num <= last_seq) {
		client_history_deq.pop_front();
	}
	
	// Case A: 오차가 5cm(0.05f) 이하면 완벽! 무시.
	if (errorDist < 0.05f) {
		return;
	}
	else if (errorDist < 0.5f) {
		// 한 번에 팍 이동하지 않고 30%씩 부드럽게 스르륵 이동 (과거 코드의 부드러움)
		position = Vector3::Add(position, error, 0.3f);
	
		// 장부(미래 예측 위치)들도 똑같이 밀어주기!
		for (auto& frame : client_history_deq) {
			frame.predicted_pos = Vector3::Add(frame.predicted_pos, error, 0.3f);
		}
		return;
	}
	
	SetPosition(serverPos);
	velocity = server_velocity;
	
	for (auto& frame : client_history_deq) {
		SimulateMove(frame.input, frame.duration);
		frame.predicted_pos = position; // 장부 갱신
	}
}