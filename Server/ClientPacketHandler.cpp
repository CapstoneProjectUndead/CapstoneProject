#include "pch.h"
#include "ClientPacketHandler.h"
#include "ClientSession.h"
#include "SceneManager.h"
#include "TestScene.h"
#include "Player.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)

bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len)
{
	cout << "정의 되지 않은 패킷 ID 입니다!" << endl;
	assert(nullptr);
	return false;
}

bool Handle_C_LOGIN(shared_ptr<Session> session, C_LOGIN& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TEST].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TEST);

	activeScene->PushPacketJob(session
		, (CTestScene*)activeScene
		, &CTestScene::EnterPlayer
		, pkt);

	return true;
}

bool Handle_C_MOVE(shared_ptr<Session> session, C_Move& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TEST].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TEST);

	activeScene->PushPacketJob(session
		, (CTestScene*)activeScene
		, &CTestScene::MovePlayer
		, pkt);
	
	return true;
}

bool Handle_C_PLAYERINPUT(shared_ptr<Session> session, C_Input& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TEST].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TEST);

	activeScene->PushPacketJob(
		session,
		(CTestScene*)activeScene,
		&CTestScene::MovePlayer,
		pkt
	);

	return true;
}
