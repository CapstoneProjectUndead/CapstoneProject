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

class CPlayer : public CCharacter {
public:
	CPlayer();
	virtual void Update(float elapsedTime) override;
	void PreUpdate(float elapsedTime);

	void SetState(const PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetDestInfo(const PlayerInfo& pos) { dest_info = pos; }
	void RecordOpponentFrameHistory(const OpponentFrameHistory& state);

	bool GetIsMyPlayer() const { return is_my_player; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

	XMFLOAT3 GetHeadPosition() const override;
public:
	// 커스터마이징용
	// 0: dog, 1: cat, 2: buddy
	//void ChangeModelSet(int setIndex);
	//void ChangeEyes(int index);
	//void ChangeMouth(int index);
	//
	//std::array<std::shared_ptr<CMaterialComponent>, 3> body_materials;
	//std::array<std::vector<std::shared_ptr<CMeshComponent>>, 3> eartail_parts;
	//std::array<std::shared_ptr<CMaterialComponent>, 3> eyes_material;
	//std::array<std::shared_ptr<CMaterialComponent>, 3> mouth_material;
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
};

