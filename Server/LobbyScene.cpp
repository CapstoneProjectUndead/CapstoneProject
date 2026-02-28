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
		obj->world_matrix = children->localMatrix;

		// =========================================================
		// [핵심 해결책] mesh.bounds가 비정상일 경우를 대비해 직접 Bounds 계산
		// =========================================================
		BoundingBox realBounds = children->mesh.bounds;

		// 1. 시각적 메쉬가 없고 충돌체만 있는 노드라면 collider 데이터로 Bounds 재계산
		if (children->mesh.positions.empty() && !children->collider.positions.empty()) {
			BoundingBox::CreateFromPoints(realBounds, children->collider.positions.size(), children->collider.positions.data(), sizeof(XMFLOAT3));
		}

		// 2. 바닥이 완전 평면(두께 0)이면 EPA 연산이 실패하므로 강제로 최소 두께(0.1f) 부여
		if (realBounds.Extents.y < 0.1f) {
			realBounds.Extents.y = 0.1f;
		}
		// =========================================================

		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;

		switch (stringToLobbyMeshName(children->name)) {
		case LobbyMeshName::Wall:
		{
			std::unique_ptr< CColliderShape> shape = std::make_unique<CConcaveMeshShape>(children->collider.positions, children->collider.indices);
			// children->mesh.bounds 대신 realBounds 사용!
			auto collider = std::make_shared<CColliderComponent>(shape, realBounds);
			CollisionFilter filter;
			filter.category = EColLayer::WALL;
			filter.mask = EColLayer::PLAYER;
			collider->SetFillter(filter);

			collider->owner = obj.get();
			collider->Update(0.0f);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		case LobbyMeshName::Floor:
		{
			// children->mesh.bounds 대신 realBounds 사용!
			std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(realBounds.Extents, realBounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, realBounds);
			CollisionFilter filter;
			filter.category = EColLayer::GROUND;
			filter.mask = EColLayer::PLAYER;
			boxCollider->SetFillter(filter);

			boxCollider->owner = obj.get();
			boxCollider->Update(0.0f);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
			break;
		}
		case LobbyMeshName::GroundPipe:
		{
			// children->mesh.bounds 대신 realBounds 사용!
			std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(realBounds.Extents, realBounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, realBounds);
			boxCollider->SetFillter(filter);

			boxCollider->owner = obj.get();
			boxCollider->Update(0.0f);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
			break;
		}
		case LobbyMeshName::Counter:
		{
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			// children->mesh.bounds 대신 realBounds 사용!
			auto collider = std::make_shared<CColliderComponent>(shape, realBounds);
			collider->SetFillter(filter);

			collider->owner = obj.get();
			collider->Update(0.0f);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		case LobbyMeshName::Unknown:
		{
			if (children->collider.positions.empty()) break;
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			// children->mesh.bounds 대신 realBounds 사용!
			auto collider = std::make_shared<CColliderComponent>(shape, realBounds);
			collider->SetFillter(filter);

			collider->owner = obj.get();
			collider->Update(0.0f);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}

		} // <-- switch문 닫는 괄호

		// 오브젝트 보관 (이전처럼 switch문 밖, for문 안에 위치!)
		static_objects.push_back(obj);
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

	// 유저 Scene에 입장
	EnterScene(player);
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

	// 유저 Scene에 입장
	// EnterScene 에서 유저들의 입장 정보들을 다 처리하도록 수정. (26. 2. 25)
	EnterScene(player);
	
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
}

CLobbyScene::LobbyMeshName CLobbyScene::stringToLobbyMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, LobbyMeshName> table = {
		{"Wall", LobbyMeshName::Wall},
		{"Floor", LobbyMeshName::Floor},
		{"GroundPipe", LobbyMeshName::GroundPipe},
		{"Stone012", LobbyMeshName::Counter}
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}