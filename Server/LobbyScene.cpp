#include "pch.h"
// Server쪽 LobbyScene
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
#include "HumanMonster.h"
#include "ServerObjectFactory.h"
#include "GameScene.h"

#undef min
#undef max


CLobbyScene::CLobbyScene(uint32 roomId)
	: CScene(SCENE_TYPE::LOBBY, roomId)
	, player_ready_cnt(0)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::Start()
{
	CreateLobby();

	// 테스트
	shared_ptr<CHumanMonster> humanMonster = static_pointer_cast<CHumanMonster>(CServerObjectFactory::CreateMonster(MON_TYPE::HUMAN_MONSTER, scene_type, GetRoom()));
	humanMonster->SetPosition(0.f, 0.1f, -1.5f);
	humanMonster->SetOriginPos({ 0.f, 0.1f, -1.5f });
	AddMonster(humanMonster);
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);

	CheckReady();
}

void CLobbyScene::CheckReady()
{
	if (auto r = room.lock()) {

		// 모든 플레이어 준비완료, GameScene으로 이동!
		const int total = r->GetCurrentPlayerCount();
		if (player_ready_cnt == total) {
			
			// GameScene 생성, 플레이어에게 GameScene Map 전송, 플레이어 GameScene으로 이동
			SendPlayerToGameScene();

			// 이 방은 게임이 시작되어서, 이제 입장 불가능
			r->SetIsGameStart(true);

			player_ready_cnt = 0;
		}
	}
}

void CLobbyScene::SendPlayerToGameScene()
{
	if (auto r = room.lock()) {

		for (auto& [id, player] : players) {

			auto session = player->GetSession();

			// GameScene은 절차적 생성 map이라,
			// 입장전에 항상 CreateGameScene을 호출한다.
			CGameScene* gameScene = (CGameScene*)r->GetScenes()[(UINT)SCENE_TYPE::GAME].get();
			//gameScene->CreateGameScene();

			// 플레이어들에게 GameScene Map 정보 패킷을 보낸다. 
			{
				S_MapStart mapStartpkt;
				auto sendBuffer = MAKE_SEND_BUFFER(mapStartpkt);
				if (session) {
					session->DoSend(sendBuffer);
				}

				auto& instanceData = gameScene->instance_data;

				const int chunkSize = 70;
				int totalDataCnt = instanceData.size();
				int currentIndex = 0;

				//while (totalDataCnt > 0) {
				//
				//	S_MapData mapDataPkt;
				//	mapDataPkt.data_count = chunkSize;
				//
				//	for (int i = 0; i < chunkSize; ++i) {
				//
				//		NetPacket::InstanceData instData{ instanceData[currentIndex].position
				//			, instanceData[currentIndex].rotationY
				//			, static_cast<NetPacket::EModelType>(instanceData[currentIndex].type) };
				//
				//		mapDataPkt.data[i] = instData;
				//		++currentIndex;
				//	}
				//	totalDataCnt -= chunkSize;
				//
				//	auto sendBuffer = MAKE_SEND_BUFFER(mapDataPkt);
				//	if (session) {
				//		session->DoSend(sendBuffer);
				//	}
				//
				//	// 남은 수가 chunkSize(70개) 보다 작다면
				//	if (totalDataCnt < chunkSize) {
				//
				//		S_MapData mapDataPkt;
				//		mapDataPkt.data_count = totalDataCnt;
				//
				//		for (int i = 0; i < totalDataCnt; ++i) {
				//			NetPacket::InstanceData instData{ instanceData[currentIndex].position
				//				, instanceData[currentIndex].rotationY
				//				, static_cast<NetPacket::EModelType>(instanceData[currentIndex].type) };
				//
				//			mapDataPkt.data[i] = instData;
				//			++currentIndex;
				//		}
				//
				//		auto sendBuffer = MAKE_SEND_BUFFER(mapDataPkt);
				//		if (session) {
				//			session->DoSend(sendBuffer);
				//		}
				//
				//		break;
				//	}
				//}

				S_MapEnd mapEndtpkt;
				sendBuffer = MAKE_SEND_BUFFER(mapEndtpkt);
				if (session) {
					session->DoSend(sendBuffer);
				}
			}
		}

		// 플레이어를 GameScene으로 이동
		vector<shared_ptr<CPlayer>> vecPlayers;

		for (auto& [id, player] : players) {
			vecPlayers.push_back(player);
		}

		for (auto& player : vecPlayers) {
			ChangeScene(player, SCENE_TYPE::GAME);

			// 플레이어에게 GameScene으로 전환하라고 알려야한다.
			S_SceneChange changeScenePkt;
			changeScenePkt.player_id = player->GetID();
			changeScenePkt.current_scene = scene_type;
			changeScenePkt.target_scene = SCENE_TYPE::GAME;

			auto sendBuffer = MAKE_SEND_BUFFER(changeScenePkt);
			if (auto session = player->GetSession()) {
				session->DoSend(sendBuffer);
			}
		}
	}
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

	// Player 생성 (플레이어 ID = 유저 ID)
	shared_ptr<CPlayer> player = CServerObjectFactory::CreatePlayerTest(SCENE_TYPE::LOBBY, session, user);

	// Player 위치 지정 (임시)
	XMFLOAT3 pos{};
	pos.x = rand() % 3 - 2;
	pos.y = 0.f;
	pos.z = rand() % 3 - 2;
	player->SetPosition(pos);

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

void CLobbyScene::Handle_C_Ready(shared_ptr<Session> session, const C_Ready& pkt)
{
	auto iter = players.find(pkt.player_id);
	assert(iter != players.end());
	iter->second->SetIsReady(true);
	++player_ready_cnt;
}

CLobbyScene::LobbyMeshName CLobbyScene::stringToLobbyMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, LobbyMeshName> table = {
		{"Wall", LobbyMeshName::Wall},
		{"Floor", LobbyMeshName::Floor},
		{"GroundPipe", LobbyMeshName::GroundPipe},
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}

void CLobbyScene::CreateLobby()
{
	// 1. 맵 파일 로드
	std::string fileName{ "../Modeling/lobby_0305.bin" };
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

		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
		obj->world_matrix = children->localMatrix;

		// =================================================
		// mesh.bounds가 비정상일 경우를 대비해 직접 Bounds 계산
		// =================================================
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

		}

		// 오브젝트 보관 (이전처럼 switch문 밖, for문 안에 위치!)
		static_objects.push_back(obj);
	}
}
