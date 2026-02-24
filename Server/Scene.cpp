#include "pch.h"
// Server쪽 Scene
#include "Scene.h"
#include "ClientSession.h"
#include "Player.h"
#include "Room.h"
#include "RoomManager.h"

CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
	, dt_ping_accumulator(0.0f)
{

}

CScene::CScene(SCENE_TYPE type, uint32 roomId)
	: scene_type(type)
	, room_id(roomId)
	, dt_ping_accumulator(0.0f)
{
}

CScene::~CScene()
{

}

void CScene::Update(const float elapsedTime)
{
	// 패킷 큐에 쌓인 메세지들을 한꺼번에 처리
	HandlePackets();
	SimulatePlayers(elapsedTime);
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
	SendPlayersResults();
	SendPlayersCheckPing();
}

void CScene::SendPlayersResults()
{
	// 시뮬레이션 돌린 플레이어의 결과를 모든 유저들에게 통보
	for (auto& [id, player] : players) {

		if (player) {
			S_Move movePkt;

			movePkt.last_seq_num = player->GetLastSequence();
			movePkt.info.player_id = player->GetID(); // "움직인 플레이어"의 ID
			movePkt.scene_type = player->GetCurrentSceneType();

			movePkt.info.x = player->GetPosition().x;
			movePkt.info.y = player->GetPosition().y;
			movePkt.info.z = player->GetPosition().z;

			movePkt.info.vx = player->GetVelocity().x;
			movePkt.info.vy = player->GetVelocity().y;
			movePkt.info.vz = player->GetVelocity().z;

			movePkt.info.yaw = player->GetYaw();
			movePkt.info.pitch = player->GetPitch();

			movePkt.info.state = player->GetState();
			movePkt.timestamp = player->GetLastSimulatedTime();

			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);
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

void CScene::SimulatePlayers(const float elapsedTime)
{
	for (auto& [id, player] : players) {
		if (player) {
			player->Update(elapsedTime);
		}
	}
}

void CScene::EnterScene(shared_ptr<CPlayer> player)
{
	players[player->GetID()] = player;
}

void CScene::LeaveScene(uint64 playerId)
{
	S_RemovePlayer removePkt;
	removePkt.player_id = playerId;
	removePkt.scene_type = scene_type;

	players.erase(playerId);

	SendBufferRef sendBuffer = MAKE_SEND_BUFFER(removePkt);
	BroadCast(sendBuffer);
}

// 서버 권위 방식
void CScene::Handle_C_Player_Input(shared_ptr<Session> session, const C_Input& pkt)
{
	auto it = players.find(pkt.info.player_id);
	if (it == players.end())
		return;

	auto mover = it->second; // 실제 움직인 플레이어

	if (pkt.seq_num <= mover->GetLastSequence())
		return;

	// 회전은 클라 권위 방식이기 때문에, 클라에서 받은 회전값을 적용한다.
	mover->SetYaw(pkt.info.yaw);
	mover->SetPitch(pkt.info.pitch);

	// 플레이어가 누른 입력과 시퀀스 넘버를 입력 큐에 저장
	InputData input{ pkt.info.w, pkt.info.a, pkt.info.s, pkt.info.d, pkt.info.space };
	PendingInput pInput{ input, pkt.seq_num };
	mover->PushInput(pInput);
}

void CScene::Handle_C_Player_Leave(shared_ptr<Session> session, const PktDummy& pkt)
{
	LeaveScene(pkt.value);

	if (auto r = room.lock()) {
		r->PlayerLeave();
	}

	// 해당 방의 씬들에 유저들이 하나도 없다면 방 삭제!
	if (!HasPlayers()) {
		if (auto r = room.lock()) {
			CRoomManager::GetInstance().DeActiveRoom(r);
		}
		//CRoomManager::GetInstance().DestroyRoomLock(room_id);
	}
}