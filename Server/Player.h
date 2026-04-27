#pragma once
// Server쪽 Player

#include "Object.h"

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

public:
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

public:
	// 캐릭터 스텟 관련 함수들
	uint32 GetMaxHp() const { return stat.maxHp; }
	uint32 GetMaxStamina() const { return stat.maxStamina; }

	uint32 GetHp()      const { return stat.hp; }
	void   SetHp(uint32 hp) { stat.hp = hp; }

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

	void   ApplyKnockback(XMFLOAT3 dir, float force);
	void   ApplyStun(float time);

private:
	void UpdateStamina(float elapsedTime);
	void ProcessMining(const InputData& input, float elapsedTime, bool isMoving);

private:
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
	uint16			  equipped_item_id;  // 0 = 맨손
	ITEM_SUB_TYPE	  equipped_item_sub_type;
	shared_ptr<CItem> equipped_item;

	PlayerStat  stat;
	float       accumulate_stamina;
	bool        stamina_exhausted;

	XMFLOAT3    knockback_vel{};
	float       knockback_timer{ 0.0f };

	float       stun_timer{ 0.0f };

	// 공중 판정 디바운스 (is_grounded 떨림 방지)
	float       grounded_timer{ 0.1f };

	// 채굴 애니메이션 유지 타이머
	float       dig_timer;
	const float DIG_DURATION = 1.03f;
};

