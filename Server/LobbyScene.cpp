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
#include "ItemFactory.h"
#include "Item.h"
#include "Inventory.h"

#undef min
#undef max

static constexpr XMFLOAT3 LOBBY_SPAWN_POINTS[] = {
	{ 0.7f, 0.f,  0.f },
	{  1.f, 0.f,  0.f },
	{ -1.f, 0.f, -0.5f },
	{  1.5f, 0.f, -1.f },
};


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
	for (auto& [id, player] : players)
		player->ApplySeparation(players);

	CScene::Update(elapsedTime);

	CheckReady();
}

void CLobbyScene::OnSceneActivate()
{
	CScene::OnSceneActivate();

	if (active_player_count > 1)
		return;

	if (auto r = room.lock()) {
		r->SetIsGameStart(false);
	}
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

void CLobbyScene::EnterScene(shared_ptr<CPlayer> player)
{
	int idx = (int)players.size() % (int)std::size(LOBBY_SPAWN_POINTS);
	player->SetPosition(LOBBY_SPAWN_POINTS[idx]);
	player->ResetAll();
	CScene::EnterScene(player);
}

void CLobbyScene::SendPlayerToGameScene()
{
	if (auto r = room.lock()) {

		// GameScene의 Map data 생성
		CGameScene* gameScene = (CGameScene*)r->GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		gameScene->CreateGameScene();

		static constexpr XMFLOAT3 GAME_SPAWN_OFFSETS[4] = {
			{ -0.3f, 0.f, -0.3f },
			{  0.3f, 0.f, -0.3f },
			{ -0.3f, 0.f,  0.3f },
			{  0.3f, 0.f,  0.3f },
		};

		XMFLOAT3 spawnBase = gameScene->FindSpawnPoint();
		int spawnIdx = 0;

		for (auto& [id, player] : players) {

			auto session = player->GetSession();

			XMFLOAT3 offset = (spawnIdx < 4) ? GAME_SPAWN_OFFSETS[spawnIdx] : XMFLOAT3{ spawnIdx * 0.3f, 0.f, 0.f };
			player->SetPosition(XMFLOAT3{ spawnBase.x + offset.x, spawnBase.y, spawnBase.z + offset.z });
			++spawnIdx;

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

				// 보물은 이제 CMineableObject로 존재하며 S_MapData로 위치가 전달된다.
				// 클라가 TREASURE 타입 instance를 보고 직접 CMineableObject를 생성한다.
				// 채굴로 파괴되는 시점에 S_SpawnItem으로 WorldItem(보물 아이템)이 스폰된다.

				// 서버가 부여한 CMineableObject world_id/위치 전송
				// 클라는 이 패킷을 받고 이미 생성된 CMineableObject들과 position 매칭으로 world_id를 세팅한다.
				{
					auto& mineableMap = gameScene->mineable_objects;
					S_MINEABLELIST_WRITE writer(SCENE_TYPE::GAME);
					auto list = writer.ReserveMineableList(static_cast<uint32>(mineableMap.size()));

					int i = 0;
					for (auto& [id, mineable] : mineableMap) {
						list[i].world_id = id;
						list[i].type = mineable->GetMineableType();
						const XMFLOAT3& pos = mineable->GetPosition();
						list[i].x = pos.x;
						list[i].y = pos.y;
						list[i].z = pos.z;
						++i;
					}

					auto mineableBuf = writer.CloseAndReturn();
					if (session) {
						session->DoSend(mineableBuf);
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

void CLobbyScene::Handle_C_ShopState(shared_ptr<Session> session, const C_ShopState& pkt)
{
	auto it = players.find(pkt.player_id);
	if (it == players.end() || !it->second)
		return;

	auto& player = it->second;

	player->SetState(PLAYER_STATE::IDLE);
	player->SetVelocity(0, 0, 0);
}

void CLobbyScene::Handle_C_BuyItem(shared_ptr<Session> session, const C_BuyItem& pkt)
{
	auto player = players[pkt.player_id];
	if (!player)
		return;

	if (pkt.qty == 0 || pkt.item_id == 0)
		return;

	auto inventory = player->GetInventory();
	if (!inventory)
		return;

	// 총액 = 클라가 계산한 개당 가격 × 수량 (개인 상점이라 가격은 클라 신뢰)
	uint32 total = pkt.unit_price * static_cast<uint32>(pkt.qty);

	// 소지금(coin) 검증: 부족하면 구매 거부
	if (player->GetCoin() < total) {
		S_UpdateCoin coinPkt;
		coinPkt.player_id = player->GetID();
		coinPkt.coin = player->GetCoin();
		coinPkt.scene_type = scene_type;
		auto sendBuffer = MAKE_SEND_BUFFER(coinPkt);
		session->DoSend(sendBuffer);
		return;
	}

	// 아이템을 생성해 서버 인벤토리에 추가 (월드에 스폰하지 않는 단순 생성)
	vector<shared_ptr<CItem>> bought;
	bought.reserve(pkt.qty);
	for (uint16 i = 0; i < pkt.qty; ++i) {
		auto item = ItemFactory::Create(pkt.item_id);
		if (!item)
			break;
		if (!inventory->AddItem(item))
			break;
		bought.push_back(item);
	}

	if (bought.empty())
		return;

	// 실제 추가된 수량만큼만 소지금 차감
	uint32 amount = player->GetCoin() - pkt.unit_price * static_cast<uint32>(bought.size());
	player->SetCoin(amount);

	S_UpdateCoin coinPkt;
	coinPkt.player_id = player->GetID();
	coinPkt.coin = amount;
	coinPkt.scene_type = scene_type;
	auto sendBuffer = MAKE_SEND_BUFFER(coinPkt);
	session->DoSend(sendBuffer);

	// S_AddItemList로 묶어서 구매자에게만 통보 (클라가 인벤토리에 추가)
	S_ADDITEMLIST_WRITE pktWriter(pkt.player_id, SCENE_TYPE::LOBBY);
	auto list = pktWriter.ReserveItemList(static_cast<uint16>(bought.size()));
	for (size_t i = 0; i < bought.size(); ++i) {
		list[i].item_id      = bought[i]->GetItemId();
		list[i].inventory_id = bought[i]->GetInventoryID();
		list[i].item_type    = bought[i]->GetItemType();
	}

	sendBuffer = pktWriter.CloseAndReturn();
	session->DoSend(sendBuffer);
}

void CLobbyScene::Handle_C_SpendCoin(shared_ptr<Session> session, const C_SpendCoin& pkt)
{
	auto it = players.find(pkt.player_id);
	if (it == players.end() || !it->second)
		return;

	auto& player = it->second;

	uint32 playerCoin = player->GetCoin();
	if (playerCoin >= pkt.spent_coin)
		player->SetCoin(playerCoin - pkt.spent_coin);

	S_UpdateCoin coinPkt;
	coinPkt.player_id = player->GetID();
	coinPkt.coin = playerCoin;
	coinPkt.scene_type = scene_type;
	auto sendBuffer = MAKE_SEND_BUFFER(coinPkt);
	session->DoSend(sendBuffer);
}

void CLobbyScene::Handle_C_RefreshStore(shared_ptr<Session> session, const C_RefreshStore& pkt)
{
	auto it = players.find(pkt.player_id);
	if (it == players.end() || !it->second)
		return;

	auto& player = it->second;

	bool success = false;
	uint32 playerCoin = player->GetCoin();
	if (playerCoin >= pkt.spent_coin) {
		player->SetCoin(playerCoin - pkt.spent_coin);
		success = true;
	}

	S_UpdateCoin coinPkt;
	coinPkt.player_id = player->GetID();
	coinPkt.coin = playerCoin;
	coinPkt.scene_type = scene_type;
	auto sendBuffer = MAKE_SEND_BUFFER(coinPkt);
	session->DoSend(sendBuffer);

	if (success) {
		S_RefreshStore freshPkt;
		freshPkt.player_id = player->GetID();
		freshPkt.scene_type = player->GetCurrentSceneType();
		sendBuffer = MAKE_SEND_BUFFER(freshPkt);
		session->DoSend(sendBuffer);
	}
}

CLobbyScene::LobbyMeshName CLobbyScene::stringToLobbyMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, LobbyMeshName> table = {
		{"Wall",        LobbyMeshName::Wall},
		{"Floor",       LobbyMeshName::Floor},
		{"GroundPipe",  LobbyMeshName::GroundPipe},
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}

void CLobbyScene::CreateLobby()
{
	std::string fileName{ "../Modeling/lobby.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	if (!frameRoot) {
		printf("Map Load Failed!\n");
		return;
	}

	for (const auto& children : frameRoot->childrens) {
		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
		obj->SetCurrentSceneType(scene_type);
		obj->world_matrix = children->local_matrix;

		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;

		switch (stringToLobbyMeshName(children->name)) {
		case LobbyMeshName::Wall:
		{
			if (!children->mesh_colliders.empty()) {
				std::unique_ptr<CColliderShape> shape = std::make_unique<CConcaveMeshShape>(children->mesh_colliders[0].positions, children->mesh_colliders[0].indices);
				auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
				collider->SetFillter({ EColLayer::WALL, EColLayer::ALL_MOB });
				obj->SetComponent(collider);
				collider->Update(0.0f);
				GetPhysicsManager()->SetCollider(collider);
			}
		}
		break;
		case LobbyMeshName::Floor:
		case LobbyMeshName::GroundPipe:
			CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, children, EColLayer::GROUND, EColLayer::ALL_MOB);
			break;
		default:
			CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, children, EColLayer::OBJECT, EColLayer::ALL_MOB);
			break;
		}

		static_objects.push_back(obj);
	}
}