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

void CScene::Update(float elapsedTime)
{
	// 패킷 큐에 쌓인 메세지들을 한꺼번에 처리
	HandlePackets();
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
