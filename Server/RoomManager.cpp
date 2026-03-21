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

void CRoomManager::CreateRoom(shared_ptr<Session> session, const C_CreateRoom& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	shared_ptr<CRoom> room = make_shared<CRoom>(pkt.room_name);

	// �濡 �����ؾ� �ϴ� ��� Scene���� �����ϰ� �ʱ�ȭ
	room->Initialize();

	uint32 roomId = room->GetRoomID();

	// �������� �ڽ��� ���� ��ID�� ������ �ִ´�.
	user->SetRoomID(roomId);

	// ������ �ڽ��� Room �����͸� ��� �ִ´�.
	user->SetRoom(room);

	// �÷��̾� Custom Scene�� ���� (�ϰ� push��)
	{
		auto& scenes = room->GetScenes();
		CCustomScene* customScene = dynamic_cast<CCustomScene*>(scenes[(UINT)SCENE_TYPE::CUSTOMS].get());
		assert(customScene);

		C_EnterRoom enterPkt;
		enterPkt.user_id = user->GetUserID();
		enterPkt.room_id = room->GetRoomID();

		customScene->PushPacketJob(session
			, (CCustomScene*)customScene
			, &CCustomScene::C_Handle_Enter_CustomScene
			, enterPkt);
	}

	// �� map�� ����
	lock_guard<mutex> lg(rooms_lock);
	rooms[room->GetRoomID()] = room;
}

void CRoomManager::EnterRoom(shared_ptr<Session> session, const C_EnterRoom& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	auto room = FindRoomLock(pkt.room_id);
	if (room) {
		if (room->IsValid()) {
			// ���� ��ID�� �� Set
			user->SetRoomID(pkt.room_id);
			user->SetRoom(room);

			// �÷��̾� Custom Scene�� ����
			auto& scenes = room->GetScenes();
			CCustomScene* customScene = dynamic_cast<CCustomScene*>(scenes[(UINT)SCENE_TYPE::CUSTOMS].get());
			assert(customScene);

			customScene->PushPacketJob(session
				, (CCustomScene*)customScene
				, &CCustomScene::C_Handle_Enter_CustomScene
				, pkt);
		}
		else {
			// ���� �ʰ� �Ǵ� �̹� ���� ������ ��
			// fail ��Ŷ ����
			S_EnterRoom enterPkt;
			enterPkt.success = false;
			enterPkt.room_id = pkt.room_id;
			auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
			if (user->GetSession())
				user->GetSession()->DoSend(sendBuffer);
		}
	}
	else {
		// ��ȿ���� ���� ��
		// fail ��Ŷ ����
		S_EnterRoom enterPkt;
		enterPkt.success = false;
		enterPkt.room_id = pkt.room_id;
		enterPkt.scene_type = SCENE_TYPE::LOBBY;
		auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}
}

void CRoomManager::LeaveAndCleanupRoom(shared_ptr<Session> session, const C_LeaveRoom& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);
	auto player = user->GetPlayer();
	assert(player);

	auto room = player->GetRoom();
	assert(room);

	// �÷��̾ �ִ� �뿡�� �÷��̾ ���� ������ �÷��̾ ����
	auto& scenes = room->GetScenes();
	CScene* scene = scenes[(UINT)player->GetCurrentSceneType()].get();
	assert(scene);

	scene->PushPacketJob(session
		, (CScene*)scene
		, &CScene::Handle_C_Player_Leave
		, pkt);
}

void CRoomManager::SendRoomList(shared_ptr<Session> session)
{
	lock_guard<mutex> lg(rooms_lock);
	if (!rooms.empty()) {

		// 게임 시작 전인 방만 카운트
		int32 validRoomCount = 0;
		for (auto& room : rooms) {
			if (!room.second->GetIsGameStart())
				validRoomCount++;
		}

		if (validRoomCount == 0) {
			S_Room_List roomPkt;
			roomPkt.room_count = 0;
			auto sendBuffer = MAKE_SEND_BUFFER(roomPkt);
			session->DoSend(sendBuffer);
		}
		else {
			S_ROOMLIST_WRITE pktWriter;
			S_ROOMLIST_WRITE::RoomList roomList = pktWriter.ReserveRoomList(validRoomCount);

			int idx = 0;
			for (auto& room : rooms) {
				if (room.second->GetIsGameStart() == true)
					continue;
				roomList[idx++] = S_Room_List::Room{ NetRoomInfo{room.second->GetRoomInfo()} };
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			session->DoSend(sendBuffer);
		}
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
