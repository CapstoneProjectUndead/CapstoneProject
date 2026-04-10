#include "pch.h"
// Server쪽 LobbyScene
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
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

void CLobbyScene::Initialize()
{
	CScene::Initialize();

	CreateLobby();
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);

	CheckReady();
}

void CLobbyScene::OnSceneActivate()
{
	CScene::OnSceneActivate();
}

void CLobbyScene::OnSceneDeactivate()
{
	CScene::OnSceneDeactivate();
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

				// 보물 위치 및 ID 전송
				{
					S_ITEMLIST_WRITE writer(SCENE_TYPE::GAME);
					auto itemList = writer.ReserveItemList((uint32)gameScene->item_manager->treasure_map.size());

					uint32 i = 0;
					for (const auto& [id, treasure] : gameScene->item_manager->treasure_map) {
						itemList[i].item_type     = ITEM_TYPE::TREASURE;
						itemList[i].item_id       = 110;
						itemList[i].item_world_id = treasure.world_id;
						itemList[i].x             = treasure.treasure_pos.x;
						itemList[i].y             = treasure.treasure_pos.y;
						itemList[i].z             = treasure.treasure_pos.z;
						++i;
					}

					if (session) {
						session->DoSend(writer.CloseAndReturn());
					}
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
	shared_ptr<CPlayer> player = CServerObjectFactory::CreatePlayerTest(SCENE_TYPE::LOBBY, session, user, GetPhysicsManager());

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

void CLobbyScene::Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt)
{
	CScene::Handle_C_Player_Leave(session, pkt);

	auto iter = players.find(pkt.user_id);
	if (iter == players.end())
		return;

	if (iter->second->GetIsReady()) {
		--player_ready_cnt;
	}
}

void CLobbyScene::Handle_C_Ready(shared_ptr<Session> session, const C_Ready& pkt)
{
	auto iter = players.find(pkt.player_id);
	assert(iter != players.end());
	iter->second->SetIsReady(true);
	++player_ready_cnt;

	S_Ready readyPkt;
	readyPkt.player_id = pkt.player_id;
	auto sendBuffer = MAKE_SEND_BUFFER(readyPkt);
	BroadCast(sendBuffer);
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
		obj->SetCurrentSceneType(scene_type);
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
			break;
		}

		}

		static_objects.push_back(obj);
	}
}
