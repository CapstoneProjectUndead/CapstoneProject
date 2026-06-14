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
	virtual void OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers) override;

	void OnAttack();	// 공격 시 호출
public:
	void SetState(const PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetDestInfo(const PlayerInfo& pos) { dest_info = pos; }
	void RecordOpponentFrameHistory(const OpponentFrameHistory& state);

	bool GetIsMyPlayer() const { return is_my_player; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

	XMVECTOR GetHeadPosition() const override;

public:
	// 캐릭터 스텟 관련 함수들
	uint32 GetMaxHp() const { return stat.maxHp; }
	uint32 GetMaxStamina() const { return stat.maxStamina; }

	uint32 GetHp() const { return stat.hp; }
	void   SetHp(const uint32 hp) 
	{ 
		stat.hp = hp; 
		if (g_is_single && hp <= 0 && state != PLAYER_STATE::DEAD) {
			state = PLAYER_STATE::DEAD;
			is_possessed = false;
			possession_timer = 0.0f;
		}
	}

	uint32 GetStamina() const { return stat.stamina; }
	void   SetStamina(const uint32 stamina) { stat.stamina = stamina; }

	bool GetIsPossessed() const { return is_possessed; }
	void SetPossessed(bool val) { is_possessed = val; }

	float GetPossessionTimer() const { return possession_timer; }
	void  SetPossessionTimer(float t) { possession_timer = t; }

	bool  GetIsStunned() const { return is_stunned; }
	void  SetStunned(bool stun) { is_stunned = stun; }

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

	// 복귀 상태 (정산 시스템)
	bool   GetReturned() const { return is_returned; }
	void   SetReturned(bool r) { is_returned = r; }

	// 빈사/사망: 무력 상태 (입력 차단 공용)
	bool   IsIncapacitated() const { return state == PLAYER_STATE::ALMOST_DEAD || state == PLAYER_STATE::DEAD; }

	// 서버가 알려준 플레이어 이름 (닉네임 표시용)
	const std::string& GetName() const { return player_name; }
	void  SetName(const std::string& n) { player_name = n; }

public:
	// 커스터마이징용
	// 0: dog, 1: cat, 2: buddy
	void ChangeModelSet(int setIndex);
	void ChangeEyes(int index) { eyes_idx = index; }
	void ChangeMouth(int index) { mouth_idx = index; }
	int GetModelTypeIndex() const { return model_type_idx; }
    int GetEyesIndex() const { return eyes_idx; }
    int GetMouthIndex() const { return mouth_idx; }

	// 커스터마이즈한 캐릭터 종류(Dog/Cat/Bunny) → 상대 상태 UI 사진 매핑용
	PLAYER_IMAGE GetPlayerImage() const { return player_image; }

	// 부위별 단일 대표 컴포넌트 
	std::shared_ptr<CMaterialComponent> body_material_comp;
	std::shared_ptr<CMaterialComponent> eartail_material_comp;
	std::shared_ptr<CMaterialComponent> eyes_material_comp;
	std::shared_ptr<CMaterialComponent> mouth_material_comp;
	// 0:Dog계열, 1:Cat계열, 2:Bunny계열 메시들을 담음 (2번 세트들도 뼈대와 외형 메시는 공유하므로 3개 유지)
	std::array<std::vector<std::shared_ptr<CMeshComponent>>, 3> eartail_parts;

	void SetDowsing(bool dows) { is_dowsing = dows; }
	bool GetDowsing() const { return is_dowsing; }
	void SetEquippedItemId(uint16 id);
	uint16 GetEquippedItemId() const { return equipped_item_id; }

	PLAYER_STATE GetLastNetState() const { return last_net_state; }
	void SetLastNetState(PLAYER_STATE s) { last_net_state = s; }

private:
	void OpponentMoveSyncByInterpolation(float elapsedTime);
	void OpponentRotateSync(float elapsedTime);
	
protected:
	uint32 room_id; // 이 플레이어가 참여하고 있는 방 ID

	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	PLAYER_STATE last_net_state = PLAYER_STATE::IDLE;
	bool is_my_player = false;
	PlayerInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값

	float smoothed_delay = 0.1f;

	// [원격 위치 One-Euro 필터 상태] 서버 메시 바닥에서 오는 좌표 노이즈 제거용
	bool     euro_init    = false;
	float    euro_last_ts = 0.0f;
	XMFLOAT3 euro_x_prev{};   // 직전 입력값
	XMFLOAT3 euro_x_hat{};    // 직전 필터 출력
	XMFLOAT3 euro_dx_hat{};   // 직전 미분 필터 출력

	std::deque<OpponentFrameHistory> interpolation_deq;
	std::vector<Buff>                buffs;

	bool							 is_dowsing = false;
	uint16							 equipped_item_id{ 0 };
	PlayerStat stat;
	PLAYER_IMAGE					 player_image = PLAYER_IMAGE::NONE;

	bool  is_possessed    = false;
	float possession_timer = 0.0f;

	bool     is_stunned    = false;
	float    stun_timer    = { 0.0f };

	bool     is_knocked_back = false;

	// 복귀 상태 (정산 시스템): 복귀존 진입 후 true. 이동/공격 입력 차단 + 몬스터 타겟 제외
	bool        is_returned{ false };

	// 서버가 알려준 플레이어 이름 (본인은 S_LOGIN, 타인은 S_SpawnPlayer/S_PLAYER_LIST)
	std::string player_name;

	// 커스터마이징 인덱스 변수
	int model_type_idx = 0; // 0:Dog, 1:Cat, 2:Bunny, 3:Dog2, 4:Cat2, 5:Bunny2
	int eyes_idx = 0;      // 0 ~ 9
	int mouth_idx = 0;     // 0 ~ 9

public:
	bool GetIsKnockedBack() const { return is_knocked_back; }
};

