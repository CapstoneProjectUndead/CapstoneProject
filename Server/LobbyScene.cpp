#include "pch.h"
// Server쪽 LobbyScene
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"

#undef min
#undef max


CLobbyScene::CLobbyScene()
    : CScene(SCENE_TYPE::LOBBY)
{
}

CLobbyScene::CLobbyScene(uint32 roomId)
	: CScene(SCENE_TYPE::LOBBY, roomId)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::Start()
{
	// 1. 맵 파일 로드
	std::string fileName{ "../Modeling/lobby_uv.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	if (!frameRoot) {
		printf("Map Load Failed!\n");
		return;
	}

	// 2. 맵 데이터를 순회하며 충돌체만 뽑아내기
	for (const auto& children : frameRoot->childrens) {

		// 위치 정보가 아예 없는 빈 노드는 스킵
		if (children->mesh.positions.empty() && children->collider.positions.empty())
			continue;

		auto obj = std::make_shared<CObject>();

		// 핵심: 이동/회전/크기(Matrix)를 그대로 서버 객체에 적용
		obj->world_matrix = children->localMatrix;

		// ============================
		// [충돌체 생성 및 물리 엔진 등록]
		// ============================

		if (children->name == "Wall") {
			std::unique_ptr< CColliderShape> shape = std::make_unique<CTriangleMeshShape>(children->collider.positions, children->collider.indices);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			collider->owner = obj.get();
			collider->Update(0.0f);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);

		}
		else {
			if (children->name == "Floor") {
				std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
				auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);				
				collider->owner = obj.get(); // 필수 (이게 없으면 허공에 뜨는 버그 발생)
				collider->Update(0.0f);
				obj->SetComponent(collider);
				CPhysicsManager::GetInstance().SetCollider(collider);

				static_objects.push_back(obj);
			}
			else if (!children->collider.positions.empty()) {
				std::unique_ptr< CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
				auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
				collider->owner = obj.get();
				collider->Update(0.0f);
				obj->SetComponent(collider);
				CPhysicsManager::GetInstance().SetCollider(collider);

				static_objects.push_back(obj);
			}
		}
	}
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	CPhysicsManager::GetInstance().Update(elapsedTime);
}

// 특정 Scene을 테스트할 때, 사용할 함수. 
void CLobbyScene::C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// User 객체 생성
	shared_ptr<CUser> user;
	if (!CAST_CS(session)->GetUser()) {
		user = make_shared<CUser>();

		// ClientSession이 Plyaer를 참조. (refcount 증가)
		CAST_CS(session)->SetUser(user);
	}

	// Player 객체 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// Player의 ID는 User ID와 동일
	player->SetID(user->GetUserID());
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// Player도 ClientSession을 약한 참조 (refcount 증가 x)
	player->SetSession(session);

	// User Player를 참조 (refcount 증가)
	user->SetPlayer(player);

	// Player도 CUser를 약한 참조 (refcount 증가 x)
	player->SetUser(CAST_CS(session)->GetUser());

	// 지금 접속한 유저에게 로그인 허락 / 플레이어 생성 패킷 보냄
	{
		S_SpawnPlayer spawnPkt;
		//spawnPkt.room_id = player->GetRoomID();
		spawnPkt.scene_type = SCENE_TYPE::LOBBY;
		spawnPkt.is_my_player = true;
		spawnPkt.info.player_id = player->GetID();
		//spawnPkt.info.room_id = player->GetRoomID();
		spawnPkt.info.is_my_player = true;
		spawnPkt.info.x = player->GetPosition().x;
		spawnPkt.info.y = player->GetPosition().y;
		spawnPkt.info.z = player->GetPosition().z;

		auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
		session->DoSend(sendBuffer);
	}

	// 지금 접속한 유저에게 다른 유저의 정보도 알려준다.
	// 여기서는 가변길이 패킷을 보낸다.
	{
		if (!players.empty()) {

			int32 cnt = players.size();
			int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

			// 예시, 꼭 LobbyScene일 필요는 없다.
			S_PLAYERLIST_WRITE pktWriter(player->GetRoomID(), SCENE_TYPE::LOBBY);

			S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

			int idx = 0;
			for (auto& pl : players) {
				if (pl.second->GetID() == player->GetID())
					continue;

				auto otherPlayer = pl.second;
				NetPlayerInfo info{ otherPlayer->GetID(), otherPlayer->GetRoomID()
					, otherPlayer->GetPosition().x, pl.second->GetPosition().y
					, pl.second->GetPosition().z };

				userList[idx++] = { info };
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			session->DoSend(sendBuffer);
		}
	}

	// 유저 Scene에 입장
	EnterScene(player);

	// 다른 유저에게 지금 접속한 유저의 정보를 알려준다.
	{
		S_SpawnPlayer spawnPkt;
		//spawnPkt.room_id = player->GetRoomID();
		spawnPkt.scene_type = SCENE_TYPE::LOBBY;
		spawnPkt.is_my_player = false;
		spawnPkt.info.player_id = player->GetID();
		//spawnPkt.info.room_id = player->GetRoomID();
		spawnPkt.info.is_my_player = false;
		spawnPkt.info.x = player->GetPosition().x;
		spawnPkt.info.y = player->GetPosition().y;
		spawnPkt.info.z = player->GetPosition().z;

		auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
		BroadCast(sendBuffer, player->GetID());
	}
}

void CLobbyScene::C_Enter_Lobby(shared_ptr<Session> session, const PktDummy& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	uint32 roomId = pkt.value;
	auto room = user->GetRoom();
	assert(room);

	// 플레이어 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어의 (플레이어 ID = 유저 ID)
	player->SetID(user->GetUserID());

	// 지금 플레이어가 속한 방ID
	player->SetRoomID(roomId);

	// 지금 플레이어가 속한 방
	player->SetRoom(room);

	// Player가 속한 Scene 설정
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 방 인원 수 증가
	room->PlayerEnter();

	// 지금 방에 입장한 유저에게 플레이어 생성 허락
	{
		S_SpawnPlayer spawnPkt;
		spawnPkt.room_id = player->GetRoomID();
		spawnPkt.scene_type = SCENE_TYPE::LOBBY;
		spawnPkt.is_my_player = true;
		spawnPkt.info.player_id = player->GetID();
		spawnPkt.info.room_id = player->GetRoomID();
		spawnPkt.info.is_my_player = true;
		spawnPkt.info.x = player->GetPosition().x;
		spawnPkt.info.y = player->GetPosition().y;
		spawnPkt.info.z = player->GetPosition().z;

		auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}

	// 유저에게 입장 허락 패킷과 방에 있는 다른 유저의 정보를 알려준다..
	// 여기서는 가변길이 패킷을 보낸다.
	{
		// 먼저 그 방에 LobbyScene에 있는 유저들 정보를 보내준다.
		if (!players.empty()) {

			int32 cnt = players.size();
			int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

			S_PLAYERLIST_WRITE pktWriter(roomId, SCENE_TYPE::LOBBY);

			S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

			int idx = 0;
			for (auto& pl : players) {
				if (pl.second->GetID() == player->GetID())
					continue;

				auto otherPlayer = pl.second;
				NetPlayerInfo info{ otherPlayer->GetID(), otherPlayer->GetRoomID()
					, otherPlayer->GetPosition().x, pl.second->GetPosition().y
					, pl.second->GetPosition().z };

				userList[idx++] = { info };
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			if (user->GetSession())
				user->GetSession()->DoSend(sendBuffer);
		}
	}
	
	// S_Enter_Room 패킷
	// 입장 허락.
	{
		S_EnterRoom enterPkt;
		enterPkt.success = true;
		enterPkt.room_id = user->GetRoomID();
		enterPkt.scene_type = player->GetCurrentSceneType();
		auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}

	// 유저 Scene에 입장
	EnterScene(player);

	// 다른 유저에게 지금 접속한 유저의 정보를 알려준다.
	{
		S_SpawnPlayer spawnPkt;
		spawnPkt.room_id = player->GetRoomID();
		spawnPkt.scene_type = SCENE_TYPE::LOBBY;
		spawnPkt.is_my_player = false;
		spawnPkt.info.player_id = player->GetID();
		spawnPkt.info.room_id = player->GetRoomID();
		spawnPkt.info.is_my_player = false;
		spawnPkt.info.x = player->GetPosition().x;
		spawnPkt.info.y = player->GetPosition().y;
		spawnPkt.info.z = player->GetPosition().z;

		auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
		BroadCast(sendBuffer, player->GetID());
	}
}