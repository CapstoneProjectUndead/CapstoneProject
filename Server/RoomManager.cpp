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

void CRoomManager::CreateRoom(const string& name, shared_ptr<CUser> user)
{
	unique_ptr<CRoom> room = make_unique<CRoom>(name);

	uint32 roomId = room->GetRoomID();
	NetRoomInfo info{ room->GetRoomInfo() };
	auto session = user->GetSession();
	assert(session);

	// 플레이어 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어의 (고유ID = 유저ID)
	player->SetID(user->GetUserID());

	// 지금 플레이어가 속한 방ID
	player->SetRoomID(roomId);

	// 지금 플레이어가 속한 Scene
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 유저에도 자신이 속한 방ID를 가지고 있는다.
	user->SetRoomID(roomId);

	// LobbyScene 생성
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY] = make_unique<CLobbyScene>();

	// 플레이어 Lobby씬 입장
	room->GetScenes()[(UINT)SCENE_TYPE::LOBBY]->EnterScene(player);

	// 방 map에 저장
	lock_guard<mutex> lg(rooms_lock);
	rooms[room->GetRoomID()] = std::move(room);

	// 유저에게 방 생성을 허락.
	S_CreateRoom createRoomPkt;
	createRoomPkt.room_info = info;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_CreateRoom>(createRoomPkt);
	session->DoSend(sendBuffer);
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

void CRoomManager::EnterRoom(shared_ptr<CUser> user, uint32 roomId)
{
	lock_guard<mutex> lg(rooms_lock);
	CRoom* room = FindRoomNoLock(roomId);
	if (room) {
		if (room->IsValid()) {
			// 유저 방ID Set
			user->SetRoomID(roomId);

			auto& scenes = room->GetScenes();
			static_cast<CLobbyScene*>(scenes[(UINT)SCENE_TYPE::LOBBY].get())->EnterLobby(user);
		}
		else {
			// fail 패킷 전송
		}
	}
	else {
		// fail 패킷 전송
	}
}

void CRoomManager::LeaveAndCleanupRoom(shared_ptr<CPlayer> player)
{
	if (player->GetRoomID() != -1) {

		auto room = FindRoomLock(player->GetRoomID());
		if (room) {
			// 플레이어가 있는 룸에서 플레이어가 속한 씬에서 플레이어를 제거
			room->GetScenes()[(UINT)player->GetCurrentSceneType()]->LeaveScene(player->GetID());

			// 해당 방의 씬들에 유저들이 하나도 없다면 방 삭제!
			if (room->SearchPlayersAllScene()) {
				rooms.erase(room->GetRoomID());
			}
		}
	}
}

CRoom* CRoomManager::FindRoomLock(uint32 roomId)
{
	lock_guard<mutex> lg(rooms_lock);
	auto iter = rooms.find(roomId);
	return iter->second.get();
}

CRoom* CRoomManager::FindRoomNoLock(uint32 roomId)
{
	auto iter = rooms.find(roomId);
	return iter->second.get();
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
