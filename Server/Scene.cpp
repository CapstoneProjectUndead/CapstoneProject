#include "pch.h"
// Server쪽 Scene
#include "Scene.h"
#include "ClientSession.h"
#include "Room.h"
#include "RoomManager.h"
#include "Monster.h"
#include "PhysicsManager.h"
#include "Collider.h"
#include "ItemManager.h"
#include "Item.h"


CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
	, room_id(-1)
	, active_player_count(0)
	, dt_ping_accumulator(0.0f)
	, monster_cnt(0)
{

}

CScene::CScene(SCENE_TYPE type, uint32 roomId)
	: scene_type(type)
	, room_id(roomId)
	, active_player_count(0)
	, dt_ping_accumulator(0.0f)
	, monster_cnt(0)
{
}

CScene::~CScene()
{
	
}

void CScene::Initialize()
{
	item_manager = std::make_unique<CItemManager>(scene_type);
}

void CScene::Update(const float elapsedTime)
{
	// 패킷 큐에 쌓인 메세지들을 한꺼번에 처리
	HandlePackets();
	SimulatePlayers(elapsedTime);
	SimulateMonsters(elapsedTime);
}

void CScene::HandlePackets()
{
	queue<Job> q;
	{
		lock_guard<mutex> lg(job_queue_lock);
		if (job_queue.empty())
			return;

		q = std::move(job_queue);
	}

	while (!q.empty()) {
		Job job = std::move(q.front());
		q.pop();
		job.Execute();
	}
}

void CScene::SendResults()
{
	SendPlayersResult();
	SendMonstersResult();
	//SendPlayersCheckPing();
}

void CScene::SendPlayersResult()
{
	// 시뮬레이션 돌린 플레이어의 결과를 모든 유저들에게 통보
	for (auto& [id, player] : players) {

		if (player) {
			S_PlayerMove movePkt;

			movePkt.last_seq_num = player->GetLastSequence();
			movePkt.info.player_id = player->GetID(); 
			movePkt.scene_type = player->GetCurrentSceneType();

			movePkt.info.is_possessed = player->GetIsPossessed();

			movePkt.info.x = player->GetPosition().x;
			movePkt.info.y = player->GetPosition().y;
			movePkt.info.z = player->GetPosition().z;

			movePkt.info.vx = player->GetVelocity().x;
			movePkt.info.vy = player->GetVelocity().y;
			movePkt.info.vz = player->GetVelocity().z;

			movePkt.info.yaw = player->GetYaw();
			movePkt.info.pitch = player->GetPitch();

			movePkt.info.state = player->GetState();
			movePkt.info.is_grounded = player->GetIsGround();
			movePkt.timestamp = player->GetLastSimulatedTime();
			movePkt.stamina = player->GetStamina();
			movePkt.hp      = player->GetHp();
			movePkt.round_timer = GetSyncedRoundTimer();

			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_PlayerMove>(movePkt);
			BroadCast(sendBuffer);
		}
	}
}

void CScene::SendMonstersResult()
{
	// 시뮬레이션 돌린 플레이어의 결과를 모든 유저들에게 통보
	for (auto& [id, monster] : monsters) {

		if (monster) {
			S_MonsterMove movePkt;

			movePkt.info.monster_id = monster->GetID(); // "������ ����"�� ID
			movePkt.scene_type = monster->GetCurrentSceneType();

			movePkt.info.x = monster->GetPosition().x;
			movePkt.info.y = monster->GetPosition().y;
			movePkt.info.z = monster->GetPosition().z;

			movePkt.info.vx = monster->GetVelocity().x;
			movePkt.info.vy = monster->GetVelocity().y;
			movePkt.info.vz = monster->GetVelocity().z;

			movePkt.info.yaw = monster->GetYaw();
			movePkt.info.pitch = monster->GetPitch();

			movePkt.info.AI_state = monster->GetAIState();
			movePkt.timestamp = monster->GetLastSimulatedTime();

			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_MonsterMove>(movePkt);
			BroadCast(sendBuffer);
		}
	}
}

void CScene::SendPlayersCheckPing()
{
	float now = g_server_total_time;

	for (auto& [id, player] : players) {
		if (player) {
			if (now - player->GetLastPingSendTime() > 2.0f) {
				auto session = player->GetSession();
				if (session) {
					player->SendPing();
					player->SetLastPingSendTime(now);
				}
			}
		}
	}
}

void CScene::BroadCast(SendBufferRef sendBuffer)
{
	for (auto& [id, player] : players) {
		if (player) {
			auto session = player->GetSession();
			if (session) {
				session->DoSend(sendBuffer);
			}
		}
	}
}

void CScene::BroadCast(SendBufferRef sendBuffer, uint64 exceptID)
{
	for (auto& [id, player] : players) {
		if (player) {

			if (player->GetID() == exceptID) 
				continue;

			auto session = player->GetSession();
			if (session) {
				session->DoSend(sendBuffer);
			}
		}
	}
}

void CScene::AddMonster(shared_ptr<CMonster> monster)
{
	monsters[monster->GetID()] = monster;
	++monster_cnt;
}

void CScene::SimulatePlayers(const float elapsedTime)
{
	for (auto& [id, player] : players) {
		if (player) {
			player->Update(elapsedTime);
		}
	}
}

void CScene::SimulateMonsters(const float elapsedTime)
{
	for (auto& [id, monster] : monsters) {
		if (monster) {
			monster->Update(elapsedTime);
		}
	}

	std::vector<uint64> toDelete;
	for (const auto& [id, monster] : monsters) {
		if (monster && monster->IsPendingDelete())
			toDelete.push_back(id);
	}

	for (uint64 id : toDelete) {
		monsters.erase(id);

		S_DeSpawnMonster despawnMonPkt;
		despawnMonPkt.monster_id = id;
		despawnMonPkt.room_id = room_id;
		despawnMonPkt.scene_type = scene_type;

		auto sendBuffer = MAKE_SEND_BUFFER(despawnMonPkt);
		BroadCast(sendBuffer);
	}
}

void CScene::EnterScene(shared_ptr<CPlayer> player)
{
	SendExistingUsers(player);

	players[player->GetID()] = player;
	player->SetCurrentSceneType(scene_type);
	player->ResetDigTimer(); // 4.19 추가.

	BroadcastUserEnter(player);

	// 3월 9일 추가
	// 몬스터가 있다면 몬스터 정보도 보내기
	{
		S_SpawnMonster spawnMonsterPkt;

		for (auto& [id, monster] : monsters) {

			spawnMonsterPkt.room_id = room_id;
			spawnMonsterPkt.scene_type = scene_type;
			spawnMonsterPkt.info.monster_id = monster->GetID();
			spawnMonsterPkt.info.monster_type = monster->GetMonsterType();
			spawnMonsterPkt.info.room_id = room_id;

			spawnMonsterPkt.info.x = monster->GetPosition().x;
			spawnMonsterPkt.info.y = monster->GetPosition().y;
			spawnMonsterPkt.info.z = monster->GetPosition().z;

			auto sendBuffer = MAKE_SEND_BUFFER(spawnMonsterPkt);
			if (player->GetUser()->GetSession())
				player->GetUser()->GetSession()->DoSend(sendBuffer);
		}
	}

	// 3월 29일 추가
	// 해당 씬에 존재하는 아이템 리스트를 입장한 유저에게 알려준다.
	if (!item_manager->GetItems().empty()) {

		S_SPAWN_ITEMLIST_WRITE writer(scene_type);
		auto itemList = writer.ReserveItemList((uint32)item_manager->GetItems().size());

		uint32 i = 0;
		for (const auto& [id, worldItem] : item_manager->GetItems()) {

			itemList[i].item_type = worldItem->item->GetItemType();
			itemList[i].item_id = worldItem->item->GetItemId();
			itemList[i].item_world_id = worldItem->world_id;
			itemList[i].x = worldItem->position.x;
			itemList[i].y = worldItem->position.y;
			itemList[i].z = worldItem->position.z;
			++i;
		}

		if (player->GetSession()) {
			player->GetSession()->DoSend(writer.CloseAndReturn());
		}
	}
}

void CScene::LeaveScene(uint64 playerId)
{
	players.erase(playerId);

	// 다른 유저들에게 유저가 나간다는 것을 알려준다.
	S_RemovePlayer removePkt;
	removePkt.player_id = playerId;
	removePkt.scene_type = scene_type;

	SendBufferRef sendBuffer = MAKE_SEND_BUFFER(removePkt);
	BroadCast(sendBuffer);
}

void CScene::OnSceneActivate()
{
	if ((scene_type == SCENE_TYPE::TITLE) || (scene_type == SCENE_TYPE::CUSTOMS))
		return;

	active_player_count++;

	// 이미 다른 플레이어가 있으면 충돌체 이미 등록됨
	if (active_player_count > 1)
		return;

	// GameScene은 CreateGameScene()에서 충돌체를 생성과 동시에 PhysicsManager에 등록하므로
	// 여기서 다시 등록하면 중복 등록된다.
	// Lobby 등 다른 씬은 Create 시점에 static_objects에만 저장하므로 여기서 등록한다.
	if (scene_type != SCENE_TYPE::GAME) {
		for (const auto& obj : static_objects) {
			CColliderComponent* col = obj->GetComponent<CColliderComponent>();
			if (col) {
				GetPhysicsManager()->SetCollider(col);
			}
		}
	}
}

void CScene::OnSceneDeactivate()
{
	if ((scene_type == SCENE_TYPE::TITLE) || (scene_type == SCENE_TYPE::CUSTOMS))
		return;

	active_player_count--;

	// 아직 다른 플레이어가 남아있으면 충돌체 유지
	if (active_player_count > 0)
		return;

	GetPhysicsManager()->EraseCollider(OBJECT_TYPE::STATIC_OBJECT, scene_type);
	GetPhysicsManager()->EraseCollider(OBJECT_TYPE::MINEABLE_OBJECT, scene_type);
	GetPhysicsManager()->EraseCollider(OBJECT_TYPE::WORLD_ITEM, scene_type);
	GetPhysicsManager()->EraseCollider(OBJECT_TYPE::MONSTER, scene_type);

	monsters.clear();
	monster_cnt = 0;
}

void CScene::SendExistingUsers(shared_ptr<CPlayer> player)
{
	// 여기서는 가변길이 패킷을 보낸다.
	if (!players.empty()) {

		int32 cnt = players.size();
		int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

		S_PLAYERLIST_WRITE pktWriter(player->GetRoomID(), scene_type);

		S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

		int idx = 0;
		for (auto& pl : players) {
			if (pl.second->GetID() == player->GetID())
				continue;

			auto otherPlayer = pl.second;
			NetPlayerInfo info{ otherPlayer->GetID(), otherPlayer->GetRoomID()
				, otherPlayer->GetBodyType(), otherPlayer->GetEyesType(), otherPlayer->GetMouthType()
				, otherPlayer->GetPosition().x, pl.second->GetPosition().y
				, pl.second->GetPosition().z };

			userList[idx++] = { info };
		}

		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		if (player->GetUser()->GetSession())
			player->GetUser()->GetSession()->DoSend(sendBuffer);
	}
}

void CScene::BroadcastUserEnter(shared_ptr<CPlayer> player)
{
	S_SpawnPlayer spawnPkt;
	spawnPkt.room_id = player->GetRoomID();
	spawnPkt.info.player_id = player->GetID();
	spawnPkt.scene_type = scene_type;
	spawnPkt.is_my_player = false;
	spawnPkt.info.is_my_player = false;
	spawnPkt.info.room_id = player->GetRoomID();
	spawnPkt.info.body_type = player->GetBodyType(); 
	spawnPkt.info.eyes_type = player->GetEyesType(); 
	spawnPkt.info.mouth_type = player->GetMouthType();
	spawnPkt.info.x = player->GetPosition().x;
	spawnPkt.info.y = player->GetPosition().y;
	spawnPkt.info.z = player->GetPosition().z;

	auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
	BroadCast(sendBuffer, player->GetID());
}

// 서버 권위 방식
void CScene::Handle_C_Player_Input(shared_ptr<Session> session, const C_Input& pkt)
{
	auto mover = CAST_CS(session)->GetUser()->GetPlayer();
	assert(mover && mover->GetID() == pkt.info.player_id);

	if (pkt.seq_num <= mover->GetLastSequence())
		return;

	// 회전은 클라 권위 방식이기 때문에, 클라에서 받은 회전값을 적용한다.
	mover->SetYaw(pkt.info.yaw);
	mover->SetPitch(pkt.info.pitch);

	// 복귀 완료 상태: 이동/점프/공격 입력은 무시 (회전·카메라만 허용)
	if (mover->GetReturned())
		return;

	// 플레이어가 누른 입력과 시퀀스 넘버를 입력 큐에 저장
	InputData input{ pkt.info.w, pkt.info.a, pkt.info.s, pkt.info.d, pkt.info.space, pkt.info.shift, pkt.info.lbtn, pkt.info.c, pkt.info.lbtn_held };
	PendingInput pInput{ input, pkt.seq_num };
	mover->PushInput(pInput);
}

void CScene::Handle_C_Player_Leave(shared_ptr<Session> session, const C_LeaveRoom& pkt)
{
	LeaveScene(pkt.user_id);

	--active_player_count;

	// 지금 나간 유저에게도 해당 씬에 있는 유저들을 삭제하라고 알려줘야함.
	for (auto it : players) {
		S_RemovePlayer removePkt;
		removePkt.player_id = it.first;
		removePkt.scene_type = scene_type;
		auto sendBuffer = MAKE_SEND_BUFFER(removePkt);
		session->DoSend(sendBuffer);
	}

	if (auto r = room.lock()) {

		// 유저가 방에서 나간다.
		r->PlayerLeave();

		// 해당 방에 유저가 한명도 없다면 방 삭제!
		if (r->GetCurrentPlayerCount() == 0) {

			// 방을 비활성화
			CRoomManager::GetInstance().DeActiveRoom(r);

			// 어차피 삭제될 방이니 몬스터 vector도 clear 해준다.
			monsters.clear();
		}

		/// 아래 함수는 Room Update를 멀티스레드로 돌렸을 때 사용한 함수.
		// 지금은 싱글 스레드로 Room Update를 하기 때문에 아래 함수는 사용해서는 안된다.
		//CRoomManager::GetInstance().DestroyRoomLock(room_id);
	}
}

void CScene::Handle_C_Scene_Change(shared_ptr<Session> session, const C_SceneChange& pkt)
{
	auto player = players[pkt.player_id];
	if (!player)
		return;

	ChangeScene(player, pkt.target_scene);
}

void CScene::ChangeScene(shared_ptr<CPlayer> player, SCENE_TYPE targetSceneType)
{
	// 기존 씬에서 처리할 것들 처리
	OnSceneDeactivate();

	auto room = player->GetRoom();
	CScene* targetScene = room->GetScenes()[(UINT)targetSceneType].get();

	// 입장할 씬에 입장 전 처리할 것들 처리
	targetScene->OnSceneActivate();

	targetScene->EnterScene(player);

	LeaveScene(player->GetID());

	// 지금 나간 유저에게도 해당 씬에 있는 유저들을 삭제하라고 알려줘야함.
	for (auto it : players) {
		S_RemovePlayer removePkt;
		removePkt.player_id = it.first;
		removePkt.scene_type = scene_type;
		auto sendBuffer = MAKE_SEND_BUFFER(removePkt);
		player->GetSession()->DoSend(sendBuffer);
	}

	S_SceneChange changeScenePkt;
	changeScenePkt.player_id    = player->GetID();
	changeScenePkt.current_scene = scene_type;
	changeScenePkt.target_scene  = targetSceneType;
	auto sendBuffer = MAKE_SEND_BUFFER(changeScenePkt);
	if (auto s = player->GetSession())
		s->DoSend(sendBuffer);
}