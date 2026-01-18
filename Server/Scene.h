#pragma once
// ServerÂÊ Scene
#include "Player.h"

class CScene
{
public:
	CScene();
	~CScene();

	virtual void Update(float elapsedTime);

	void BroadCast(SendBufferRef sendBuffer);
	void BroadCast(SendBufferRef sendBuffer, uint64 exceptID);

	void EnterScene(shared_ptr<CPlayer> player);
	map<uint64, shared_ptr<CPlayer>>& GetPlayers() { return players; }

protected:
	mutex								lock;
	map<uint64, shared_ptr<CPlayer>>	players;
};

