#pragma once
// Server쪽 Scene
#include "Player.h"
#include "Job.h"

class CScene
{
	template<typename T, typename PacketType>
	using memFunc = void (T::*)(shared_ptr<Session>, const PacketType&);

public:
	CScene(SCENE_TYPE type);
	~CScene();

	virtual void Update(float elapsedTime);
	virtual void EnterScene(shared_ptr<CPlayer> player);
	virtual void LeaveScene(uint64 playerId);

	void BroadCast(SendBufferRef sendBuffer);
	void BroadCast(SendBufferRef sendBuffer, uint64 exceptID);

	SCENE_TYPE GetSceneType() const { return scene_type; }
	map<uint64, shared_ptr<CPlayer>>& GetPlayers() { return players; }

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

protected:
	mutex								players_lock;
	map<uint64, shared_ptr<CPlayer>>	players;

	mutex								job_queue_lock;
	queue<Job>							job_queue;

private:
	SCENE_TYPE							scene_type;
};

