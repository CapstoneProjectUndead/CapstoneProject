#include "stdafx.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "TestScene.h"
#include "Player.h"
#include "MyPlayer.h"
#include "GeometryLoader.h"
#include "Camera.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_SS(session) static_pointer_cast<CServerSession>(session)

bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len)
{
	//std::cout << "� 吏  ⑦ ID !" << std::endl;
	assert(nullptr);
	return false;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{

	return true;
}

bool Handle_S_MYPLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	std::unique_ptr<FrameNode> frame;
	frame = CGeometryLoader::LoadGeometry("../Modeling/undead_char.bin", gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

	std::shared_ptr<CMyPlayer> myPlayer = std::make_shared<CMyPlayer>();
	myPlayer->SetID(pkt.info.id);
	myPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
	myPlayer->SetMesh(frame->mesh);

	Material m{};
	m.name = "Red";
	m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	m.roughness = 0.5f;
	m.metallic = 0.1f;
	myPlayer->SetMaterial(m);

	myPlayer->CreateConstantBuffers(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(gGameFramework.GetDevice().Get());
	scene->GetShaders().push_back(std::move(shader));

	// 移대 媛泥 
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	std::shared_ptr<CCamera> camera = std::make_shared<CCamera>();
	camera->SetViewport(0, 0, width, height);
	camera->SetScissorRect(0, 0, width, height);
	camera->GenerateProjectionMatrix(1.0f, 500.0f, (float)width / (float)height, 90.0f);
	camera->SetCameraOffset(XMFLOAT3(0.0f, 2.0f, -5.0f));
	camera->SetTarget(myPlayer.get());

	camera->CreateConstantBuffers(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());
	myPlayer->SetSession(session);

	scene->SetPlayer(myPlayer);
	scene->SetCamera(camera);

	return true;
}

bool Handle_S_ADDPLAYER(std::shared_ptr<Session> session, S_AddPlayer& pkt)
{
	std::unique_ptr<FrameNode> frame;
	frame = CGeometryLoader::LoadGeometry("../Modeling/undead_char.bin", gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

	std::shared_ptr<CMyPlayer> otherPlayer = std::make_shared<CMyPlayer>();
	otherPlayer->SetID(pkt.info.id);
	otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
	otherPlayer->SetMesh(frame->mesh);

	Material m{};
	m.name = "Red";
	m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	m.roughness = 0.5f;
	m.metallic = 0.1f;
	otherPlayer->SetMaterial(m);

	otherPlayer->CreateConstantBuffers(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	scene->EnterScene(otherPlayer, otherPlayer->GetID());

	return true;
}

bool Handle_S_PLAYERLIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	for (int i = 0; i < pkt.player_count; ++i) {

		// ㅻⅨ � 
		std::shared_ptr<CMyPlayer> otherPlayer = std::make_shared<CMyPlayer>();

		// ㅻⅨ � ID 遺
		otherPlayer->SetID(userList[i].info.id);

		// ㅻⅨ � 移 遺
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// ㅻⅨ � mesh ㅼ
		std::unique_ptr<FrameNode> frame;
		frame = CGeometryLoader::LoadGeometry("../Modeling/undead_char.bin", gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());
		otherPlayer->SetMesh(frame->mesh);

		Material m{};
		m.name = "Red";
		m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
		m.roughness = 0.5f;
		m.metallic = 0.1f;
		otherPlayer->SetMaterial(m);

		otherPlayer->CreateConstantBuffers(gGameFramework.GetDevice().Get(), gGameFramework.GetCommandList().Get());

		// Active Scene ㅻⅨ � 
		scene->EnterScene(otherPlayer, otherPlayer->GetID());
	}

	return true;
}

bool Handle_S_REMOVEPLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
		CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
		if (scene != nullptr) {
			for (auto& player : scene->GetObjects()) {
				if (player->GetID() == pkt.info.id)
					scene->LeaveScene(player->GetID());
			}
		}
	}

	return true;
}

bool Handle_S_MOVE(std::shared_ptr<Session> session, S_Move& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	auto& vec = scene->GetObjects();
	auto& indexMap = scene->GetIDIndex();
	std::shared_ptr<CMyPlayer> myPlayer =  scene->GetMyPlayer();

	// 내 플레이어이면 처리하지 않고 나가기
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.id)
		return true;

	// 해당 ID가 존재하는 플레이어인지 확인
	auto it = indexMap.find(pkt.info.id);
	if (it == indexMap.end())
		return true;

	uint64 idx = it->second;
	if (idx >= vec.size())
		return true;

	auto player = std::static_pointer_cast<CPlayer>(vec[idx]);

	ObjectInfo info;
	info.id = pkt.info.id;
	info.state = pkt.info.state;
	info.x = pkt.info.x;
	info.y = pkt.info.y;
	info.z = pkt.info.z;
	info.yaw = pkt.info.yaw;
	info.pitch = pkt.info.pitch;
	info.roll = pkt.info.roll;

	player->SetDestInfo(info);

	return true;
}
