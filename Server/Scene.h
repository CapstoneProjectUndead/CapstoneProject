#pragma once
// Server쪽 Scene
#include "Player.h"
#include "Job.h"
#include "User.h"

class CRoom;

class CScene
{
	template<typename T, typename PacketType>
	using memFunc = void (T::*)(shared_ptr<Session>, const PacketType&);

public:
	CScene(SCENE_TYPE type);
	CScene(SCENE_TYPE type, uint32 roomId);
	~CScene();

	virtual void Start() {};
	virtual void Update(const float elapsedTime);
	virtual void EnterScene(shared_ptr<CPlayer> player);
	virtual void LeaveScene(uint64 playerId);

	void SendResults();
	void SendPlayersResults();
	void SendPlayersCheckPing();

	void BroadCast(SendBufferRef sendBuffer);
	void BroadCast(SendBufferRef sendBuffer, uint64 exceptID);

private:
	void SimulatePlayers(const float elapsedTime);

	// 입장 유저에게 기존 유저들의 정보를 알려준다.
	void SendExistingUsers(shared_ptr<CPlayer> player);

	// 기존 유저에게 지금 접속한 유저의 정보를 알려준다.
	void BroadcastUserEnter(shared_ptr<CPlayer> player);

public:
	// Scene에 플레이어가 있는지 체크
	bool HasPlayers()
	{
		if (!players.empty())
			return true;
		else
			return false;
	}

	SCENE_TYPE GetSceneType() const { return scene_type; }
	map<uint64, shared_ptr<CPlayer>>& GetPlayers() { return players; }

	std::weak_ptr<CRoom>      GetRoomWeak() const { return room; }
	std::shared_ptr<CRoom>    GetRoom() const { return room.lock(); }
	void					  SetRoom(std::shared_ptr<CRoom> _room) { room = _room; }

public:
	// IOCP 스레드들이 호출 (패킷 받자마자 실행)
	template<typename T, typename PacketType>
	void PushPacketJob(shared_ptr<Session> session
		, T* obj
		, memFunc<T, PacketType> func
		, const PacketType& pkt)
	{
		lock_guard<std::mutex> lg(job_queue_lock);
		job_queue.emplace([session, obj, func, pkt]() {
			(obj->*func)(session, pkt);
			});
	}

	// IOCP 워커 스레드가 받아둔 패킷들을 여기서 로직에 반영
	void HandlePackets();

public:
	// 서버 권한 + 클라 예측 기반 Move
	void Handle_C_Player_Input(shared_ptr<Session> session, const C_Input& pkt);
	void Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt);

protected:
	map<uint64, shared_ptr<CPlayer>>	players;

	mutex								job_queue_lock;
	queue<Job>							job_queue;

	weak_ptr<CRoom>						room;
	uint32								room_id;

private:
	SCENE_TYPE							scene_type;
	float								dt_ping_accumulator;
};

