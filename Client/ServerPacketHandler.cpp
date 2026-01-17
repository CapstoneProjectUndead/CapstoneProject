#include "stdafx.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "TestScene.h"
#include "Player.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_SS(session) static_pointer_cast<CServerSession>(session)

bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len)
{
	//std::cout << "정의 되지 않은 패킷 ID 입니다!" << std::endl;
	assert(nullptr);
	return false;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{

	return true;
}

bool Handle_S_MYPLAYER(std::shared_ptr<Session> session, S_MyPlayer& pkt)
{
	std::shared_ptr<CPlayer> player = std::make_shared<CPlayer>(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());
	player->SetID(pkt.info.id);
	player->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	scene->SetPlayer(player);
	scene->SetCamera(player->GetCameraPtr());

	return true;
}

bool Handle_S_ADDPLAYER(std::shared_ptr<Session> session, S_AddPlayer& pkt)
{
	std::shared_ptr<CPlayer> player = std::make_shared<CPlayer>(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());
	player->SetID(pkt.info.id);
	player->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	scene->GetOtherPlayers().push_back(player);

	return true;
}

bool Handle_S_PLAYERLIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저 생성
		std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

		// 다른 유저 ID 부여
		otherPlayer->SetID(userList[i].info.id);

		// 다른 유저 위치 부여
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// Active Scene에 다른 유저 입장
		CSceneManager::GetInstance().GetActiveScene()->GetOtherPlayers().push_back(otherPlayer);
	}

	return true;
}
