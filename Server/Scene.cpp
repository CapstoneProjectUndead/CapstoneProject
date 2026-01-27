#include "pch.h"
// Server쪽 Scene
#include "Scene.h"
#include "ClientSession.h"
#include "Player.h"

CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
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
}

void CScene::SendPlayersResults()
{
	// 시뮬레이션 돌린 플레이어의 결과를 모든 유저들에게 통보
	for (auto& [id, player] : players)
	{
		S_Move movePkt;

		movePkt.last_seq_num = player->GetLastSequence();
		movePkt.info.id = player->GetID(); // "움직인 사람"의 ID
		movePkt.info.x = player->GetPosition().x;
		movePkt.info.y = player->GetPosition().y;
		movePkt.info.z = player->GetPosition().z;
		movePkt.info.yaw = player->GetYaw();
		movePkt.info.pitch = player->GetPitch();

		// 현재 입력 상태 (애니메이션용)
		movePkt.info.w = player->GetInput().w;
		movePkt.info.a = player->GetInput().a;
		movePkt.info.s = player->GetInput().s;
		movePkt.info.d = player->GetInput().d;

		movePkt.info.state = player->GetState();

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);
		player->GetSession()->DoSend(sendBuffer);

		// [중요] 시퀀스 번호는 오직 '움직인 본인'에게만 의미가 있음
		// 받는 사람이 mover일 때만 시퀀스를 넣어주고, 나머지에겐 0을 보냅니다.
		movePkt.last_seq_num = 0;

		// 움직인 플레이어를 제외한 나머지 유저에게 전달.
		BroadCast(sendBuffer, player->GetID());
	}
}

void CScene::BroadCast(SendBufferRef sendBuffer)
{
	lock_guard<mutex> lg(players_lock);
	for (auto& player : players)
		player.second->GetSession()->DoSend(sendBuffer);
}

void CScene::BroadCast(SendBufferRef sendBuffer, uint64 exceptID)
{
	lock_guard<mutex> lg(players_lock);
	for (auto& player : players) {
		if (player.second->GetID() == exceptID) continue;
		player.second->GetSession()->DoSend(sendBuffer);
	}
}

void CScene::SimulatePlayers(const float elapsedTime)
{
	for (auto& [id, player] : players) {
		player->Update(elapsedTime);
	}
}

void CScene::EnterScene(shared_ptr<CPlayer> player)
{
	lock_guard<mutex> lg(players_lock);
	players[player->GetID()] = player;
}

void CScene::LeaveScene(uint64 playerId)
{
	lock_guard<mutex> lg(players_lock);
	players.erase(playerId);

	S_RemovePlayer removePkt;
	removePkt.info.id = playerId;
	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_RemovePlayer>(removePkt);
	
	for (auto& player : players)
		player.second->GetSession()->DoSend(sendBuffer);
}
