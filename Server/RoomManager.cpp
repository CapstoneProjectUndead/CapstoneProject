#include "pch.h"
#include "RoomManager.h"
#include "Scene.h"
#include "User.h"
#include "CustomScene.h"
#include "LobbyScene.h"


CRoomManager::CRoomManager()
{
}

CRoomManager::~CRoomManager()
{
}

void CRoomManager::Initialize()
{

}

void CRoomManager::Update(const float elapsedTime)
{
	CheckEmptyRoom();

	lock_guard<mutex> lg(rooms_lock);
	for (auto& [id, room] : rooms)
	{
		room->Update(elapsedTime);
	}
}

void CRoomManager::SendResults()
{
	lock_guard<mutex> lg(rooms_lock);
	for (auto& [id, room] : rooms)
	{
		room->SendResults();
	}
}

void CRoomManager::CheckEmptyRoom()
{
	lock_guard<mutex> lg(rooms_lock);
	for (auto it = rooms.begin(); it != rooms.end(); ) {
		auto& room = it->second;

		if (!room->IsActive()) {
			it = rooms.erase(it);
		}
		else {
			++it;
		}
	}
}

void CRoomManager::DeActiveRoom(uint32 roomId)
{
	//lock_guard<mutex> lg(rooms_lock);
	rooms[roomId]->SetActive(false);
}

void CRoomManager::DeActiveRoom(shared_ptr<CRoom> room)
{
	room->SetActive(false);
}

shared_ptr<CRoom>CRoomManager::FindRoomLock(uint32 roomId)
{
	lock_guard<mutex> lg(rooms_lock);
	if (rooms.empty())
		return nullptr;

	auto iter = rooms.find(roomId);
	if (iter != rooms.end())
		return iter->second;
}

shared_ptr<CRoom> CRoomManager::FindRoomNoLock(uint32 roomId)
{
	if (rooms.empty())
		return nullptr;

	auto iter = rooms.find(roomId);
	if (iter != rooms.end())
		return iter->second;
}

void CRoomManager::DestroyRoomLock(uint32 roomId)
{
	lock_guard<mutex> lg(rooms_lock);
	rooms.erase(roomId);
}

void CRoomManager::DestroyRoomNoLock(uint32 roomId)
{
	rooms.erase(roomId);
}

void CRoomManager::CreateRoom(const string& name, shared_ptr<CUser> user)
{
	shared_ptr<CRoom> room = make_shared<CRoom>(name);

	// 방에 존재해야 하는 모든 Scene들을 생성하고 초기화
	room->Initialize();

	uint32 roomId = room->GetRoomID();
	NetRoomInfo info{ room->GetRoomInfo() };
	auto session = user->GetSession();
	assert(session);

	// 유저에도 자신이 속한 방ID를 가지고 있는다.
	user->SetRoomID(roomId);

	// 유저는 자신의 Room 포인터를 들고 있는다.
	user->SetRoom(room);

	// 플레이어 Custom Scene에 입장
	{
		auto& scenes = room->GetScenes();
		CScene* scene = scenes[(UINT)SCENE_TYPE::CUSTOMS].get();
		assert(scene);

		PktDummy dummypkt;
		dummypkt.value = roomId;

		scene->PushPacketJob(user->GetSession()
			, (CCustomScene*)scene
			, &CCustomScene::C_Enter_CustomScene
			, dummypkt);
	}

	// 방 map에 저장
	lock_guard<mutex> lg(rooms_lock);
	rooms[room->GetRoomID()] = room;
}

void CRoomManager::EnterRoom(shared_ptr<Session> session, uint32 roomId)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	auto room = FindRoomLock(roomId);
	if (room) {
		if (room->IsValid()) {
			// 유저 방ID와 방 Set
			user->SetRoomID(roomId);
			user->SetRoom(room);

			// 플레이어 Lobby Scene에 입장
			auto& scenes = room->GetScenes();
			CScene* scene = scenes[(UINT)SCENE_TYPE::LOBBY].get();
			assert(scene);

			PktDummy dummypkt;
			dummypkt.value = roomId;

			scene->PushPacketJob(user->GetSession()
				, (CLobbyScene*)scene
				, &CLobbyScene::C_Enter_Lobby
				, dummypkt);
		}
		else {
			// 정원 초과 또는 이미 게임 시작한 방
			// fail 패킷 전송
			S_EnterRoom enterPkt;
			enterPkt.success = false;
			enterPkt.room_id = roomId;
			enterPkt.scene_type = SCENE_TYPE::LOBBY;
			auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
			if (user->GetSession())
				user->GetSession()->DoSend(sendBuffer);
		}
	}
	else {
		// 유효하지 않은 방
		// fail 패킷 전송
		S_EnterRoom enterPkt;
		enterPkt.success = false;
		enterPkt.room_id = roomId;
		enterPkt.scene_type = SCENE_TYPE::LOBBY;
		auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}
}

void CRoomManager::LeaveAndCleanupRoom(shared_ptr<CPlayer> player)
{
	auto room = player->GetRoom();
	assert(room);

	// 플레이어가 있는 룸에서 플레이어가 속한 씬에서 플레이어를 제거
	auto& scenes = room->GetScenes();
	CScene* scene = scenes[(UINT)player->GetCurrentSceneType()].get();
	assert(scene);

	PktDummy dummyPkt;
	dummyPkt.value = player->GetID();

	scene->PushPacketJob(player->GetSession()
		, (CScene*)scene
		, &CScene::Handle_C_Player_Leave
		, dummyPkt);
}

void CRoomManager::SendRoomList(shared_ptr<Session> session)
{
	lock_guard<mutex> lg(rooms_lock);
	if (!rooms.empty()) {

		int32 roomCount = rooms.size();
		int32 pktSize = sizeof(S_Room_List) + sizeof(S_Room_List::Room) * roomCount;

		S_ROOMLIST_WRITE pktWriter;
		S_ROOMLIST_WRITE::RoomList roomList = pktWriter.ReserveRoomList(roomCount);

		int idx = 0;
		for (auto& room : rooms) {

			if (room.second->GetIsGameStart() == true)
				continue;

			roomList[idx++] = S_Room_List::Room{ NetRoomInfo{room.second->GetRoomInfo()} };
		}

		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		session->DoSend(sendBuffer);
	}
	else {
		S_Room_List roomPkt;
		roomPkt.room_count = 0;
		auto sendBuffer = MAKE_SEND_BUFFER(roomPkt);
		session->DoSend(sendBuffer);
	}
}

void CRoomManager::ProcessEvents()
{
	while (!events.empty())
	{
		events.front()();
		events.pop();
	}
}
