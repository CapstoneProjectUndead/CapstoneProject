#pragma once
// Server쪽 Player

#include "Object.h"
#include <MapGenerator/MapGenerator.h>

struct ServerFrameHistory
{
	uint64		 seq_num;
	InputData	 input{false, false, false, false, false, false};
	XMFLOAT3	 position;
	PLAYER_STATE state;
	float		 timestamp; 
};

struct PendingInput 
{
	InputData	input;
	uint64		seq_num;

	PendingInput() = default;
	PendingInput(InputData& _input, uint64 seqNum)
		: input(_input)
		, seq_num(seqNum)
	{ }
};

class CUser;
class CRoom;
class CollisionInfo;
class CInventory;
class CItem;

class CPlayer : public CObject
{
public:
	CPlayer();
	virtual ~CPlayer() override;

	void Update(const float elapsedTime) override;
	void ProcessInputQueue(const float elapsedTime);
	void SimulateMove(const InputData& input, float elapsedTime, bool updateState = true);

	// 게임씬 입장할 때, 아이템 지급 (1회성)
	bool get_items_once = false;

public:
	void SetGuset(bool g) { is_guest = g; }
	bool GetIsGuest() const { return is_guest; }

	// 로그인 계정 ID (저장 키). 유저에서 복사해 둔다 → 연결 끊김으로 CUser가 먼저 소멸해도 저장 가능.
	void SetAccountId(const string& id) { account_id = id; }
	const string& GetAccountId() const { return account_id; }

	void SetLastSequence(uint64 lastSeq) { last_processed_seq = lastSeq; }
	uint64 GetLastSequence() const { return last_processed_seq; }

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	std::weak_ptr<CUser>      GetUserWeak() const { return user; }
	std::shared_ptr<CUser>    GetUser() const { return user.lock(); }
	void SetUser(shared_ptr<CUser> _user) { user = _user; }

	std::shared_ptr<CInventory> GetInventory() const { return inventory; }
	void SetInventory(shared_ptr<CInventory> inven) { inventory = inven; }

	void RecordServerFrameHistory(const ServerFrameHistory& history);
	bool FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult);

	deque<ServerFrameHistory>& GetFrameHistoryDeq() { return server_history_deq; }
	void PushInput(const PendingInput& input) { input_queue.push_back(input); }

	// 클라의 핑 측정
	void  SendPing();
	float GetLastPingSendTime() const { return dt_ping_accumulator; };
	void  SetLastPingSendTime(float elapsedTime) { dt_ping_accumulator = elapsedTime; };
	void  UpdatePing(float newRtt)
	{
		// 급격한 변화를 막기 위해 기존 값과 섞음 (보통 9:1 비율)
		ping = (ping * 0.9f) + (newRtt * 0.1f);
	}

	uint8 GetBodyType() const { return body_type; }
	void SetBodyType(uint8 type) { body_type = type; }

	uint8 GetEyesType() const { return eyes_type; }
	void SetEyesType(uint8 type) { eyes_type = type; }

	uint8 GetMouthType() const { return mouth_type; }
	void SetMouthType(uint8 type) { mouth_type = type; }

	bool GetIsReady() const { return is_ready; }
	void SetIsReady(bool ready) { is_ready = ready; }

	void ResetAll();

public:
	// 캐릭터 스텟 관련 함수들
	uint32 GetMaxHp() const { return stat.maxHp; }
	uint32 GetMaxStamina() const { return stat.maxStamina; }

	uint32 GetHp()      const { return stat.hp; }
	void   SetHp(uint32 hp) 
	{
		stat.hp = hp;
		if (hp <= 0 && state != PLAYER_STATE::ALMOST_DEAD && state != PLAYER_STATE::DEAD) {
			state = PLAYER_STATE::ALMOST_DEAD;
			is_possessed = false;
			possession_timer = 0.0f;
			is_stunned = false;
			stun_timer = 0.0f;
		}
	}

	uint32 GetStamina() const { return stat.stamina; }
	void   SetStamina(uint32 stamina) { stat.stamina = stamina; }
	void   AddStamina(uint32 amount);

	void   SetEquipedItem(shared_ptr<CItem> item) { equipped_item = item; }
	shared_ptr<CItem> GetEquipedItem() const { return equipped_item; }

	uint16 GetEquippedItemId() const { return equipped_item_id; }
	void SetEquippedItemId(uint16 id) { equipped_item_id = id; }

	ITEM_SUB_TYPE GetEquippedItemSubType() const { return equipped_item_sub_type; }
	void		  SetEquippedItemSubType(ITEM_SUB_TYPE type) { equipped_item_sub_type = type; }

	void   ResetDigTimer() { dig_timer = 0.f; }

	void   ApplySeparation(const map<uint64, shared_ptr<CPlayer>>& allPlayers, float scale = 1.0f);

	// 넉백 + 스턴 (기본값 1.3f) 스턴은 원치 않으면 인자를 0으로 쓰면 된다.
	void   ApplyKnockback(XMFLOAT3 dir, float force, float stun_duration = 1.5f);
	void   ApplyStun(float time);
	void   ApplyPossession();

	bool  GetIsPossessed() const { return is_possessed; }
	void  SetPossessed(bool val) { is_possessed = val; }

	bool  GetIsStunned() const { return is_stunned; }

	float GetPossessionTimer() const { return possession_timer; }
	void  SetPossessionTimer(float t) { possession_timer = t; }

	bool GetDowsing() const { return is_dowsing; }
	void SetDowsing(bool dows) { is_dowsing = dows; }

	// 코인
	uint32 GetGold() const { return gold; }
	void   SetGold(uint32 amount) { gold = amount; }
	void   AddGold(uint32 amount) { gold += amount; }

	// 복귀 상태 (정산 시스템)
	bool   GetReturned() const { return is_returned; }
	void   SetReturned(bool r) { is_returned = r; }

	// 빈사/사망: 무력 상태 (몬스터 타겟 제외 + 입력/패킷 차단 공용)
	bool   IsIncapacitated() const { return state == PLAYER_STATE::ALMOST_DEAD || state == PLAYER_STATE::DEAD; }

private:
	void UpdateStamina(float elapsedTime);

	// 채굴
	void ProcessVisibleObjectMining(const InputData& input, float elapsedTime, bool isMoving);
	void ProcessUnVisibleObjectMining(const InputData& input, float elapsedTime, bool isMoving);
	void ProcessToolMining();
	void ProcessBareHandMining(float elapsedTime, const InputData& input, bool isMoving);

	// 공격
	void ProcessMeleeAttack(const InputData& input, float elapsedTime);
	void ProcessRangedAttack(const InputData& input, float elapsedTime);

	// 장착 장비 내구도 소모 (채굴/공격 공용). 내구도 0이면 제거 후 true 반환
	bool ConsumeEquippedDurability();

	// 빙의
	void UpdatePossession(float elapsedTime);
	void UpdateCHoldAction(const InputData& input, const float elapsedTime);

	XMFLOAT3            GetRandomPossessedTarget();
	shared_ptr<CPlayer> FindNearestOtherPlayer();

	void SprayAttack(float elapsedTime);
	void ApplySprayHitWhilePossessed(const XMFLOAT3& fromPos);
	void MeleeAttack();

private:
	bool						is_guest;
	string						account_id;
	weak_ptr<CUser>				user;
	uint64						last_processed_seq;
	float						ping;
	float						dt_ping_accumulator;
	PLAYER_STATE				state;
	deque<PendingInput>			input_queue;
	deque<ServerFrameHistory>	server_history_deq;

	// 커스터마이징
	uint8 body_type{};
	uint8 eyes_type{};
	uint8 mouth_type{};

	shared_ptr<CInventory>      inventory;

	bool			  is_ready;
	bool			  is_dowsing;
	uint16			  equipped_item_id;  // 0 = 맨손
	ITEM_SUB_TYPE	  equipped_item_sub_type;
	shared_ptr<CItem> equipped_item;

	PlayerStat	 stat;
	float		 accumulate_stamina;
	bool		 stamina_exhausted;
				 
	XMFLOAT3	 knockback_vel;
	float		 knockback_timer;
				 
	bool		 is_stunned;
	float		 stun_timer;

	bool         is_possessed;
	float        possession_timer;
	float        c_hold_timer;
	CHOLD_TARGET c_hold_target_type{ CHOLD_TARGET::NONE };  // 현재 C-홀드 중인 대상 종류 (POSSESSION/RESCUE)
	bool         last_c_input;
	InputData    last_input{ false, false, false, false, false, false };  // 마지막으로 수신한 입력 (빈 틱에 키 홀드 상태 유지용)
	float        input_hold_timer{ 1000.0f };                             // 마지막 입력 수신 후 경과 시간 (첫 수신 전에는 유지 비활성)
	bool         start_jump;
	float        jump_buffer_timer{ 0.0f };  // 점프 버퍼: 공중 틱에 도착한 space를 기억했다가 착지 시 점프
	int          possessed_spray_hit_count;
	float        bare_hand_dig_timer;

	std::vector<MapGenerator::Cell> possessed_nav_path;
	float    possessed_path_refresh_timer;
	XMFLOAT3 possessed_wander_target;
	bool     possessed_is_waiting;
	float    possessed_wait_timer;
	float    possession_contact_timer;

	XMFLOAT3    separation_push{};

	// 공중 판정 디바운스 (is_grounded 떨림 방지)
	float       grounded_timer{ 0.1f };

	// 소지금
	uint32      gold{ 0 };

	// 복귀 상태 (정산 시스템): 복귀존 진입 후 true. 이동/공격 입력 차단 + 몬스터 타겟 제외
	bool        is_returned{ false };

	// 채굴 애니메이션 유지 타이머
	float       dig_timer;
	const float DIG_DURATION = 1.03f;

	// 스프레이 공격 상태 유지 타이머
	float       ranged_attack_timer{ 0.0f };
	const float SPRAY_ATTACK_DURATION = 1.6f;
	const float MAGIC_ATTACK_DURATION = 1.5f;
	const float GUN_ATTACK_DURATION = 1.5f;

	// 근접 공격 상태 유지 타이머
	float       melee_attack_timer{ 0.0f };
	const float MELEE_ATTACK_DURATION = 1.5f;

	float       dig_sound_timer = -1.0f;

	// 맨손 채굴 사운드: 홀딩 중 일정 간격마다 PlaySound 패킷 전송
	float       bare_hand_sound_timer = -1.0f;
	const float BARE_HAND_SOUND_INTERVAL = 0.5f;
};

