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

	// (임시)
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

		const int total = r->GetCurrentPlayerCount();
		if (total > 0 && player_ready_cnt == total) {
			
			SendPlayerToGameScene();

			r->SetIsGameStart(true);

			player_ready_cnt = 0;
		}
	}
}

void CLobbyScene::SendPlayerToGameScene()
{
	if (auto r = room.lock()) {

		CPhysicsManager::GetInstance().EraseCollider(OBJECT_TYPE::STATIC_OBJECT);
		CPhysicsManager::GetInstance().EraseCollider(OBJECT_TYPE::MONSTER);

		// GameScene의 Map data 생성
		CGameScene* gameScene = (CGameScene*)r->GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		gameScene->CreateGameScene();

		for (auto& [id, player] : players) {

			auto session = player->GetSession();
			player->SetPosition(XMFLOAT3{ 0.0f, 2.0f, 0.0f });

			// GameScene Map 데이터를 클라이언트로 전송
			{
				S_MapStart mapStartpkt;
				auto sendBuffer = MAKE_SEND_BUFFER(mapStartpkt);
				if (session) {
					session->DoSend(sendBuffer);
				}

				auto& mapInstanceData = gameScene->map_instance_data;

				const int chunkSize = 60;
				int totalDataCnt = mapInstanceData.size();
				int currentIndex = 0;

				while (totalDataCnt > 0) {
				
					S_MapData mapDataPkt;
					mapDataPkt.data_count = chunkSize;
				
					for (int i = 0; i < chunkSize; ++i) {
				
						NetPacket::InstanceData instData{ mapInstanceData[currentIndex].position
							, mapInstanceData[currentIndex].rotationY
							, static_cast<NetPacket::EModelType>(mapInstanceData[currentIndex].type)
							, mapInstanceData[currentIndex].model };
				
						mapDataPkt.data[i] = instData;
						++currentIndex;
					}
					totalDataCnt -= chunkSize;
				
					auto sendBuffer = MAKE_SEND_BUFFER(mapDataPkt);
					if (session) {
						session->DoSend(sendBuffer);
					}
				
					// chunkSize(60)
					if (totalDataCnt < chunkSize) {
				
						S_MapData mapDataPkt;
						mapDataPkt.data_count = totalDataCnt;
				
						for (int i = 0; i < totalDataCnt; ++i) {
							NetPacket::InstanceData instData{ mapInstanceData[currentIndex].position
								, mapInstanceData[currentIndex].rotationY
								, static_cast<NetPacket::EModelType>(mapInstanceData[currentIndex].type)
								, mapInstanceData[currentIndex].model };
				
							mapDataPkt.data[i] = instData;
							++currentIndex;
						}
				
						auto sendBuffer = MAKE_SEND_BUFFER(mapDataPkt);
						if (session) {
							session->DoSend(sendBuffer);
						}
				
						break;
					}
				}

				S_MapEnd mapEndtpkt;
				sendBuffer = MAKE_SEND_BUFFER(mapEndtpkt);
				if (session) {
					session->DoSend(sendBuffer);
				}
			}
		}

		// 플레이어 GameScene으로 이동
		vector<shared_ptr<CPlayer>> vecPlayers;

		for (auto& [id, player] : players) {
			vecPlayers.push_back(player);
		}

		for (auto& player : vecPlayers) {
			ChangeScene(player, SCENE_TYPE::GAME);

			// 클라이언트에게 GameScene으로 전환해야함을 알림.
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

void CLobbyScene::C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// User ��ü ����
	shared_ptr<CUser> user;
	if (!CAST_CS(session)->GetUser()) {
		user = make_shared<CUser>();

		// ClientSession�� Plyaer�� ����. (refcount ����)
		CAST_CS(session)->SetUser(user);
	}

	// Player ���� (�÷��̾� ID = ���� ID)
	shared_ptr<CPlayer> player = CServerObjectFactory::CreatePlayerTest(SCENE_TYPE::LOBBY, session, user);

	// Player ��ġ ���� (�ӽ�)
	XMFLOAT3 pos{};
	pos.x = rand() % 3 - 2;
	pos.y = 0.f;
	pos.z = rand() % 3 - 2;
	player->SetPosition(pos);

	// ���� ������ �������� �α��� ��� / �÷��̾� ���� ��Ŷ ����
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

	// ���� Scene�� ����
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
	std::string fileName{ "../Modeling/lobby_0305.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	if (!frameRoot) {
		printf("Map Load Failed!\n");
		return;
	}

	for (const auto& children : frameRoot->childrens) {

		if (children->mesh.positions.empty() && children->collider.positions.empty())
			continue;

		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
		obj->world_matrix = children->localMatrix;

		BoundingBox realBounds = children->mesh.bounds;

		if (children->mesh.positions.empty() && !children->collider.positions.empty()) {
			BoundingBox::CreateFromPoints(realBounds, children->collider.positions.size(), children->collider.positions.data(), sizeof(XMFLOAT3));
		}

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
			// children->mesh.bounds realBounds!
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
			// children->mesh.bounds realBounds
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
			// children->mesh.bounds realBounds 
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
			// children->mesh.bounds realBounds!
			auto collider = std::make_shared<CColliderComponent>(shape, realBounds);
			collider->SetFillter(filter);

			collider->owner = obj.get();
			collider->Update(0.0f);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}

		}

		static_objects.push_back(obj);
	}
}
