#include "pch.h"
#include "Scene.h"
// ServerÂÊ TestScene
#include "ClientSession.h"
#include "Player.h"

CScene::CScene()
{

}

CScene::~CScene()
{

}

void CScene::Update(float elapsedTime)
{

}

void CScene::BroadCast(SendBufferRef sendBuffer)
{
	lock_guard<mutex> lg(lock);
	for (auto& player : players)
		player.second->GetSession()->DoSend(sendBuffer);
	
}

void CScene::BroadCast(SendBufferRef sendBuffer, uint64 exceptID)
{
	lock_guard<mutex> lg(lock);
	for (auto& player : players) {
		if (player.second->GetID() == exceptID) continue;
		player.second->GetSession()->DoSend(sendBuffer);
	}
}

void CScene::EnterScene(shared_ptr<CPlayer> player)
{
	lock_guard<mutex> lg(lock);
	players[player->GetID()] = player;
}