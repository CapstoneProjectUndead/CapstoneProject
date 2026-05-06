#include "stdafx.h"
#include "MyPlayer.h"
#include "KeyManager.h"
#include "ImGuiManager.h"

#include "ServerPacketHandler.h"
#include "NetworkManager.h"
#include "NetworkClockManager.h"
#include "SoundManager.h"
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

	// (싱글) 스테미나 Update
	if (g_is_single) {
		UpdateStamina(elapsedTime);
	}

	// (싱글) 우클릭: 퀵슬롯에 등록된 소비 아이템 사용 
	UseItem();

	// "E" 키를 누르면 인벤토리를 열고/닫기
	if (current_scene_type == SCENE_TYPE::GAME && KEY_TAP(KEY::E)) {
		CKeyManager::GetInstance().SetMouseMode(!CKeyManager::GetInstance().GetMouseMode());
		inventory->ToggleOpen();
	}

	if (KEY_TAP(KEY::F)) {

		if (g_is_single) {
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
		else {
			C_EquipItem equipPkt;

			if (!is_dowsing) {
				equipPkt.player_id = GetID();
				equipPkt.item_id = -1;
				equipPkt.is_dowsing_rod = true;
				equipPkt.scene_type = current_scene_type;
			}
			else {
				equipPkt.player_id = GetID();
				equipPkt.item_id = -1;
				equipPkt.is_dowsing_rod = false;
				equipPkt.scene_type = current_scene_type;
			}

			auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
			if (auto s = session.lock()) {
				s->DoSend(sendBuffer);
			}
		}
	}

	// 넉백
	if (knockback_timer > 0.0f) {

		float ratio = knockback_timer / 0.3f;
		velocity.x += knockback_vel.x * ratio;
		velocity.z += knockback_vel.z * ratio;

		knockback_timer -= elapsedTime;

		if (knockback_timer < 0.0f) {
			knockback_timer = 0.0f;
			is_knocked_back = false;
		}
	}

	// 스턴
	if (stun_timer > 0.0f) {
		stun_timer -= elapsedTime;

		if (stun_timer < 0.0f) {
			stun_timer = 0.0f;
			is_stunned = false;
		}
	}

	// 빙의
	if (g_is_single && is_possessed)
		UpdatePossession(elapsedTime);

	CPlayer::Update(elapsedTime);
}

void CMyPlayer::PreUpdate(float elapsedTime)
{
	ServerAuthorityMove(elapsedTime);
}

void CMyPlayer::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
	CObject::OnCollect(renderers);

	auto animator = GetComponent<CAnimatorComponent>();
	if (animator) {

		// 싱글 모드일 때는 바로 장착
		if (g_is_single) {
			auto itemFinder = GetComponent<CItemFinder>();
			if (itemFinder && itemFinder->is_enable) {
				animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_R, NULL, "dowsing_rod_0307");
				animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_L, NULL, "dowsing_rod_0307");
			}
			else if(itemFinder && !itemFinder->is_enable)
				animator->RenderSocketModel(CAnimatorComponent::HAND_R, quick_slot->GetSelectedItemId());
		}
		else {
			// 멀티 모드에서는 서버의 허락을 받는다.
			if (is_dowsing) {
				auto itemFinder = GetComponent<CItemFinder>();
				if (itemFinder && itemFinder->is_enable) {
					animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_R, NULL, "dowsing_rod_0307");
					animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_L, NULL, "dowsing_rod_0307");
				}
			}
			else if (!is_dowsing && equipped_item_id != 0)
				animator->RenderSocketModel(CAnimatorComponent::HAND_R, equipped_item_id);
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
	// 상태(state)는 서버가 결정한 값을 그대로 따른다 (Handle_S_Move_Player에서 SetState)
	// 여기서는 위치 보간만 담당
	XMFLOAT3 destPos = { dest_info.x, dest_info.y, dest_info.z };

	float lerpSpeed = 8.0f * elapsedTime;
	position = Vector3::Lerp(position, destPos, lerpSpeed);
}

void CMyPlayer::ServerAuthorityMove(const float elapsedTime)
{
	// 1. 입력 캡처
	CaptureInput(current_input);

	// 2. 회전
	ProcessRotation();

	// 3. 점프 시작 판정 + 효과음 (싱글/멀티 공통, 서버 허락 없이 즉시)
	start_jump = (current_input.space && is_grounded && !stamina_exhausted && !is_possessed);
	if (start_jump)
		CSoundManager::GetInstance().Play(SOUND_ID::jump12);

	// 4. 예측 이동 (지금은 완전히 서버 권한 방식이라 싱글 전용이 됨)
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
	currentInput.lbtn  = KEY_TAP(KEY::LBTN) && !ImGui::GetIO().WantCaptureMouse
										    && current_scene_type == SCENE_TYPE::GAME;
	currentInput.c     = KEY_PRESSED(KEY::C);
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
	// 플레이어가 몬스터 공격을 받아서 넉백 상태이면 return
	if (is_knocked_back || is_stunned || is_possessed)
		return;

	auto move = GetComponent<CMovementComponent>();
	if (!move || current_scene_type == SCENE_TYPE::CUSTOMS) return;
	
	// 키 처리
	XMFLOAT3 dir{ 0.f, 0.f, 0.f };
	if (input.w) dir.z++;
	if (input.s) dir.z--;
	if (input.a) dir.x--;
	if (input.d) dir.x++;

	if (input.space && !stamina_exhausted) {
		move->Jump();
	}

	// 상태 update
	bool isMoving = (dir.x != 0 || dir.z != 0);

	// 움찔거리는 거 방지
	grounded_timer = is_grounded ? 0.1f : (grounded_timer - dt);

	if (grounded_timer > 0.0f) {
		if (!isMoving) {
			if(state != PLAYER_STATE::DIG)
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
	inputPkt.info.lbtn  = input.lbtn;
	inputPkt.info.c     = input.c;

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

void CMyPlayer::UseItem()
{
	if (g_is_single
		&& KEY_TAP(KEY::RBTN)
		&& !ImGui::GetIO().WantCaptureMouse)
	{
		ITEM_TYPE itemType = quick_slot->GetSelectedItemType();

		// 아이템 타입이 소비 또는 기타
		if (itemType == ITEM_TYPE::CONSUMABLE || itemType == ITEM_TYPE::ETC) {

			// 퀵슬롯에 등록된 아이템의 인벤토리 ID를 가져온다.
			int invId = quick_slot->GetSelectedInvId();
			if (invId >= 0) {

				uint32 uInvId = static_cast<uint32>(invId);
				auto& items = inventory->GetItems();

				// 인벤토리에서 아이템을 찾아온다.
				auto it = items.find(uInvId);
				if (it != items.end()) {

					// 아이템 사용
					if (it->second->Use(this)) {

						// 사용한 아이템 인벤토리에서 제거
						inventory->RemoveItem(uInvId);
					}
				}
			}
		}
	}
	else if (!g_is_single
		&& current_scene_type == SCENE_TYPE::GAME
		&& KEY_TAP(KEY::RBTN)
		&& !ImGui::GetIO().WantCaptureMouse)
	{
		ITEM_TYPE itemType = quick_slot->GetSelectedItemType();
		if (itemType == ITEM_TYPE::CONSUMABLE || itemType == ITEM_TYPE::ETC) {

			int invId = quick_slot->GetSelectedInvId();

			if (invId >= 0) {

				uint32 uInvId = static_cast<uint32>(invId);
				auto& items = inventory->GetItems();
				auto it = items.find(uInvId);

				if (it != items.end()) {
					C_UseItem useItemPkt;
					useItemPkt.player_id = GetID();
					useItemPkt.item_id = it->second->GetItemId();
					useItemPkt.inventory_id = uInvId;
					useItemPkt.scene_type = GetCurrentSceneType();

					auto sendBuffer = MAKE_SEND_BUFFER(useItemPkt);
					if (auto session = GetSession()) {
						session->DoSend(sendBuffer);
					}
				}
			}
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

void CMyPlayer::SetStaminaFromServer(uint32 stamina)
{
	accumulate_stamina = static_cast<float>(stamina);
	stat.stamina = stamina;

	// 서버 값 기준으로 exhausted 플래그 동기화
	if (stamina == 0)
		stamina_exhausted = true;
	else if (stamina >= 30 && stamina_exhausted)
		stamina_exhausted = false;
}

void CMyPlayer::AddStamina(uint32 amount)
{
	accumulate_stamina = std::min(accumulate_stamina + static_cast<float>(amount),
		static_cast<float>(stat.maxStamina));

	stat.stamina = static_cast<uint32>(accumulate_stamina);
	if (stamina_exhausted && accumulate_stamina >= 30.0f)
		stamina_exhausted = false;
}

void CMyPlayer::UpdateStamina(float elapsedTime)
{
	const float drainPerSec    =  16.7f;  // 뛸 때 초당 감소 (6초면 바닥)
	const float regenPerSec    =  10.0f;   // 쉴 때 초당 회복 
	const float recoverThreshold = 30.0f; // 이 값 이상 회복돼야 다시 달리기 허용

	if (start_jump) {
		constexpr float jumpCost = 12.0f;
		accumulate_stamina -= jumpCost;
		if (accumulate_stamina <= 0.0f) {
			accumulate_stamina = 0.0f;
			stamina_exhausted = true;
		}
	}

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
		if (!is_grounded)
			return;

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

void CMyPlayer::ApplyKnockback(XMFLOAT3 dir, float force, float stun_duration)
{
	dir.y = 0.0f;
	float len = Vector3::Length(dir);

	if (len < 0.001f)
		return;

	knockback_vel    = { dir.x / len * force, 0.0f, dir.z / len * force };
	knockback_timer  = 0.3f;

	is_knocked_back  = true;
	if (stun_duration > 0.0f)
		ApplyStun(stun_duration);
}

void CMyPlayer::ApplyStun(float time)
{
	is_stunned = true;
	stun_timer = time;
	SetState(PLAYER_STATE::IDLE);
}

void CMyPlayer::ApplyPossession()
{
    is_possessed = true;
    possession_timer = 20.0f;

    possessed_nav_path.clear();
    possessed_path_refresh_timer = 0.0f;
    possessed_wander_target      = GetRandomPossessedTarget();
    possessed_is_waiting         = false;
    possessed_wait_timer         = 0.0f;
}

void CMyPlayer::UpdatePossession(float elapsedTime)
{
    possession_timer -= elapsedTime;

    if (possession_timer <= 0.0f) {

		CSoundManager::GetInstance().Play(SOUND_ID::devil_laugh1);
        possession_timer = 0.0f;
        is_possessed     = false;
        velocity.x       = 0.0f;
        velocity.z       = 0.0f;
        state            = PLAYER_STATE::IDLE;

        return;
    }

    constexpr float TILE_SIZE     = 2.0f;
    constexpr float ARRIVE_DIST   = 0.4f;
    constexpr float POSSESS_SPEED = 3.0f;

    if (possessed_is_waiting) {

        velocity.x = 0.0f;
        velocity.z = 0.0f;

        possessed_wait_timer -= elapsedTime;

        if (possessed_wait_timer <= 0.0f) {
            possessed_wander_target      = GetRandomPossessedTarget();
            possessed_nav_path.clear();
            possessed_path_refresh_timer = 0.0f;
            possessed_is_waiting         = false;
        }

        return;
    }

    XMFLOAT3 dirVec = Vector3::Subtract(possessed_wander_target, position);
    dirVec.y = 0.0f;
    float dist = Vector3::Length(dirVec);

    if (dist < ARRIVE_DIST) {

        velocity.x           = 0.0f;
        velocity.z           = 0.0f;
        possessed_is_waiting = true;
        possessed_wait_timer = 0.3f + (rand() % 5) * 0.1f;

        return;
    }

    possessed_path_refresh_timer += elapsedTime;

    if (possessed_path_refresh_timer >= 0.2f || possessed_nav_path.empty()) {

        possessed_path_refresh_timer = 0.0f;

        int sx = (int)roundf(position.x / TILE_SIZE);
        int sz = (int)roundf(position.z / TILE_SIZE);
        int ex = (int)roundf(possessed_wander_target.x / TILE_SIZE);
        int ez = (int)roundf(possessed_wander_target.z / TILE_SIZE);

        possessed_nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
    }

    XMFLOAT3 moveDir = dirVec;
    if (!possessed_nav_path.empty()) {
        XMFLOAT3 wpWorld = { possessed_nav_path[0].x * TILE_SIZE, position.y, possessed_nav_path[0].y * TILE_SIZE };
        XMFLOAT3 toWp    = Vector3::Subtract(wpWorld, position);
        toWp.y = 0.0f;
        if (Vector3::Length(toWp) > 0.1f)
            moveDir = toWp;
    }

    float moveYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
    SetYaw(moveYaw);
    SetYawPitch(moveYaw, 0.0f);

    velocity.x = look.x * POSSESS_SPEED;
    velocity.z = look.z * POSSESS_SPEED;
    state      = PLAYER_STATE::RUN;
}

XMFLOAT3 CMyPlayer::GetRandomPossessedTarget()
{
    const float TILE_SIZE = 2.0f;
    int cx = (int)roundf(position.x / TILE_SIZE);
    int cz = (int)roundf(position.z / TILE_SIZE);

    const int dx[] = { 0, 0, -1, 1 };
    const int dz[] = { -1, 1,  0, 0 };

    std::vector<MapGenerator::Cell> candidates;

    for (int i = 0; i < 4; i++) {

        int nx = cx + dx[i];
        int nz = cz + dz[i];

        if (MapGenerator::IsWalkableFloor(nx, nz) && !MapGenerator::IsBlockedStructure(nx, nz))
            candidates.push_back({ nx, nz });
    }

    if (candidates.empty())
        return position;

    int idx = rand() % (int)candidates.size();
    return { candidates[idx].x * TILE_SIZE, position.y, candidates[idx].y * TILE_SIZE };
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