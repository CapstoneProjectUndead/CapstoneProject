#pragma once
#include "Character.h"

class CCamera;
class CMaterialComponent;
class CMeshComponent;

struct OpponentFrameHistory 
{
	uint64	 player_id;
	float	 server_timestamp; // 서버에서 찍어준 도장
	XMFLOAT3 position;
	PLAYER_STATE state;
};

class CPlayer : public CCharacter 
{
public:
	CPlayer();
	virtual void Update(float elapsedTime) override;
	void PreUpdate(float elapsedTime);

public:
	void SetState(const PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetDestInfo(const PlayerInfo& pos) { dest_info = pos; }
	void RecordOpponentFrameHistory(const OpponentFrameHistory& state);

	bool GetIsMyPlayer() const { return is_my_player; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

	XMFLOAT3 GetHeadPosition() const override;

public:
	// 캐릭터 스텟 관련 함수들
	uint32 GetMaxHp() const { return stat.maxHp; }
	uint32 GetMaxStamina() const { return stat.maxStamina; }

	uint32 GetHp() const { return stat.hp; }
	void   SetHp(const uint32 hp) { stat.hp = hp; }

	uint32 GetStamina() const { return stat.stamina; }
	void   SetStamina(const uint32 stamina) { stat.stamina = stamina; }

	uint16 GetMiningSpeed() const { return stat.miningSpeed; }
	void   SetMiningSpeed(const uint16 speed) { stat.miningSpeed = speed; }

	void  AddBuff(const Buff& buff);
	void  UpdateBuffs(float elapsedTime);

	float GetMiningSpeedMult() const
	{
		float mult = 1.f;
		for (const auto& buff : buffs)
			mult *= buff.miningSpeedMult;
		return mult;
	}

private:
	void OpponentMoveSyncByInterpolation(float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

protected:
	uint32 room_id; // 이 플레이어가 참여하고 있는 방 ID

	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	
	PlayerInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값

	float smoothed_delay = 0.1f;

	std::deque<OpponentFrameHistory> interpolation_deq;
	std::vector<Buff>                buffs;

	PlayerStat stat;
};

