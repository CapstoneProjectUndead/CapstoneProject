#pragma once
// Server쪽 Scene
#include "Player.h"
#include "Job.h"
#include "User.h"
#include "ServerObjectFactory.h"
#include "ItemManager.h"

class CRoom;
class CMonster;
class CPhysicsManager;
class CItem;

class CScene
{
	template<typename T, typename PacketType>
	using memFunc = void (T::*)(shared_ptr<Session>, const PacketType&);

public:
	CScene(SCENE_TYPE type);
	CScene(SCENE_TYPE type, uint32 roomId);
	virtual ~CScene();

	virtual void Initialize();
	virtual void Update(const float elapsedTime);
	virtual void EnterScene(shared_ptr<CPlayer> player);
	virtual void LeaveScene(uint64 playerId);

	virtual void OnSceneActivate();
	virtual void OnSceneDeactivate();

	void SendResults();
	void SendPlayersResult();
	void SendMonstersResult();
	void SendPlayersCheckPing();

	void BroadCast(SendBufferRef sendBuffer);
	void BroadCast(SendBufferRef sendBuffer, uint64 exceptID);

	void AddMonster(shared_ptr<CMonster> monster);

private:
	void SimulatePlayers(const float elapsedTime);
	void SimulateMonsters(const float elapsedTime);

	// 입장 유저에게 기존 유저들의 정보를 알려준다.
	void SendExistingUsers(shared_ptr<CPlayer> player);

	// 기존 유저에게 지금 접속한 유저의 정보를 알려준다.
	void BroadcastUserEnter(shared_ptr<CPlayer> player);

protected:
	void ChangeScene(shared_ptr<CPlayer> player, SCENE_TYPE targetSceneType);

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
	map<uint64, shared_ptr<CMonster>>& GetMonsters() { return monsters; }

	weak_ptr<CRoom>      GetRoomWeak() const { return room; }
	shared_ptr<CRoom>    GetRoom() const { return room.lock(); }
	void					  SetRoom(std::shared_ptr<CRoom> _room) { room = _room; }

	weak_ptr<CPhysicsManager>   GetPhysicsManagerWeak() const { return physics_manager; }
	shared_ptr<CPhysicsManager> GetPhysicsManager() const { return physics_manager.lock(); }
	void						SetPhysicsManager(shared_ptr<CPhysicsManager> manager) { physics_manager = manager; }

	CItemManager* GetItemManager() const { return item_manager.get(); }

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
	virtual void Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt);
	void Handle_C_Scene_Change(shared_ptr<Session> session, const C_SceneChange& pkt);

	virtual void Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt) {};
	virtual void Handle_C_Drop_Item(shared_ptr<Session> session, const C_DropItem& pkt) {}
	virtual void Handle_C_Equip_Item(shared_ptr<Session> session, const C_EquipItem& pkt) {}
	virtual void Handle_C_Use_Item(shared_ptr<Session> session, const C_UseItem& pkt) {}

protected:
	map<uint64, shared_ptr<CPlayer>>	players;
	map<uint64, shared_ptr<CMonster>>	monsters;
	SCENE_TYPE							scene_type;

	mutex								job_queue_lock;
	queue<Job>							job_queue;

	weak_ptr<CRoom>						room;
	uint32								room_id;

	// 맵의 바닥, 장애물 등 움직이지 않는 정적 충돌체들을 보관하는 곳
	vector<shared_ptr<CObject>>			static_objects;

	// Room의 PhysicsManager를 약한 참조
	weak_ptr<CPhysicsManager>			physics_manager;

	// 아이템 관리 Manager
	unique_ptr<CItemManager>            item_manager;

	uint16								active_player_count;
	uint32								monster_cnt;

private:
	float								dt_ping_accumulator;
};

