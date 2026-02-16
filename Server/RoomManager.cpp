#include "pch.h"
#include "RoomManager.h"
#include "Scene.h"
#include "LobbyScene.h"
#include "User.h"


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
	// 락은 '방 목록'을 복사하거나 순회할 때만 짧게 건다
	vector<CRoom*> activeRooms;
	{
		lock_guard<mutex> lg(rooms_lock);
		for (auto& pair : rooms) 
			activeRooms.push_back(pair.second.get());
	}
	
	// 각 방의 업데이트를 스레드 풀에 던진다
	for (auto& room : activeRooms)
	{
		GThreadManager->PushTask([room, elapsedTime]() {
			room->Update(elapsedTime);
			});
	}
}

void CRoomManager::SendResults()
{
	vector<CRoom*> activeRooms;
	{
		lock_guard<mutex> lg(rooms_lock);
		for (auto& pair : rooms)
			activeRooms.push_back(pair.second.get());
	}
	
	for (auto& room : activeRooms)
	{
		GThreadManager->PushTask([room]() {
			room->SendResults();
			});
	}
}

CRoom* CRoomManager::FindRoomLock(uint32 roomId)
{
	lock_guard<mutex> lg(rooms_lock);
	auto iter = rooms.find(roomId);
	if (!iter->second)
		assert(nullptr);

	return iter->second.get();
}

CRoom* CRoomManager::FindRoomNoLock(uint32 roomId)
{
	auto iter = rooms.find(roomId);
	if (!iter->second)
		assert(nullptr);

	return iter->second.get();
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
	unique_ptr<CRoom> room = make_unique<CRoom>(name);

	// 방에 존재해야 하는 모든 Scene들을 생성하고 초기화
	room->Initialize();

	uint32 roomId = room->GetRoomID();
	NetRoomInfo info{ room->GetRoomInfo() };
	auto session = user->GetSession();
	assert(session);

	// 유저에도 자신이 속한 방ID를 가지고 있는다.
	user->SetRoomID(roomId);

	// 유저는 자신의 Room 포인터를 들고 있는다.
	user->SetRoom(room.get());

	// 플레이어 Lobby Scene에 입장
	{
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

	// 방 map에 저장
	lock_guard<mutex> lg(rooms_lock);
	rooms[room->GetRoomID()] = std::move(room);
}

void CRoomManager::EnterRoom(shared_ptr<Session> session, uint32 roomId)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	CRoom* room = FindRoomLock(roomId);
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
	uint32 roomId = player->GetRoomID();

	if (roomId != -1) {

		lock_guard<mutex> lg(rooms_lock);
		auto room = FindRoomNoLock(roomId);
		if (room) {

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

			//// 해당 방의 씬들에 유저들이 하나도 없다면 방 삭제!
			//if (room->SearchPlayersAllScene()) {
			//	DestroyRoomNoLock(roomId);
			//}
		}
	}
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
