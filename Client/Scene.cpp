#include "stdafx.h"
#include "Camera.h"
#include "Timer.h"
#include "GeometryLoader.h"
#include "KeyManager.h"
#include "Player.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "Scene.h"
#include "GameFramework.h"
#include "ServerSession.h"
#include "MeshComponent.inl"
#include "Texture.h"
#include "Mesh.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "User.h"
#include "NetworkClockManager.h"


CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
{

}

CScene::~CScene()
{

}

void CScene::ReleaseUploadBuffers()
{
	for (const auto& obj : objects) {
		obj->ReleaseUploadBuffer();
	}
}

void CScene::AnimateObjects(float elapsedTime)
{
	if (my_player) {
		my_player->Update(elapsedTime);
	}

	for (const auto& obj : objects) {
		obj->Update(elapsedTime);
	}
}

void CScene::Update(float elapsedTime)
{
	AnimateObjects(elapsedTime);

	if(camera)
		camera->Update(my_player->position, elapsedTime);
	if(light)
		light->Update(camera.get());
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (const auto& shader : shaders) {
		shader.second->RenderBegin(commandList);

		if (camera)
			camera->UpdateShaderVariables(commandList);

		if (light)
			light->Render(commandList);

		for (const auto& obj : objects) {
			if (shader.first == obj->GetShader()) {
				shader.second->Render(commandList, obj.get());
			}
		}

		if (my_player) {
			if (shader.first == my_player->GetShader()) {
				my_player->UpdateShaderVariables(commandList);
				my_player->Render(commandList);
			}
		}

		shader.second->RenderEnd(commandList);
	}
}

void CScene::EnterScene(std::shared_ptr<CObject> obj, UINT id)
{
	id_To_Index[id] = objects.size();
	objects.push_back(obj);
}

void CScene::LeaveScene(UINT id)
{
	UINT idx = id_To_Index[id];
	UINT last = objects.size() - 1;

	std::swap(objects[idx], objects[last]);
	id_To_Index[objects[idx]->GetID()] = idx;

	objects.pop_back();
	id_To_Index.erase(id);
}

void CScene::Handle_S_Spawn_Player(std::shared_ptr<Session>& session, const S_SpawnPlayer& pkt)
{
	if (pkt.is_my_player) {

		my_player = std::make_shared<CMyPlayer>();
		{
			my_player->Initialize(GET_DEVICE, GET_CMD_LIST);

			// 세션 저장
			my_player->SetSession(session);
			// 아이디 저장
			my_player->SetID(pkt.info.player_id);
			// 방 ID 저장
			my_player->SetRoomID(pkt.room_id);
			// 서버가 알려준 위치에 spawn
			my_player->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
			// 플레이어가 속한 Scene
			my_player->SetCurrentSceneType(pkt.scene_type);

			if (SERVER_SESSION) {
				auto user = SERVER_SESSION->GetUser();
				if (user) {
					// user는 player를 강한 참조
					SERVER_SESSION->GetUser()->SetMyPlayer(my_player);
					SERVER_SESSION->GetUser()->SetRoomID(pkt.room_id);

					// player는 user를 약한 참조
					my_player->SetUser(SERVER_SESSION->GetUser());
				}
			}
		}

		{
			// static shader
			std::shared_ptr<CShader> shader = std::make_unique<CShader>();
			shader->CreateShader(GET_DEVICE);
			shaders.emplace("static", std::move(shader));
		}

		{
			// skinning
			std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
			shader->CreateShader(GET_DEVICE);
			shaders.emplace("skinning", std::move(shader));
		}

		{
			std::string fileName{ "../Modeling/lobby_uv.bin" };
			auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

			CDescriptorHeapManager* heapManager{ shaders["static"]->GetHeapManager() };
			CMaterialManager matManager{};
			CTextureManager texManager{};

			for (const auto& children : frameRoot->childrens) {
				if (children->mesh.positions.empty()) break;
				auto obj = std::make_shared<CObject>();
				// 1) MeshComponent 생성
				auto meshComp = std::make_shared<CMeshComponent>();
				obj->SetComponent(meshComp);
				meshComp->SetMeshFromFile<CMatVertex>(GET_DEVICE, GET_CMD_LIST, children);
				obj->world_matrix = children->localMatrix;

				// 2) MaterialComponent 생성
				auto matComp = std::make_shared<CMaterialComponent>();
				obj->SetComponent(matComp);

				std::string name{ children->mesh.materials[0].albedoMap };
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
				auto mat = matManager.GetMeterial(name, tex);
				matComp->SetMaterial(mat);

				// 3) MeshRendererComponent 생성
				obj->SetComponent(std::make_shared<CMeshRendererComponent>());

				if (children->name == "Floor") {
					// 4) ColliderComponent 생성
					std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents);
					auto boxCollider = std::make_shared<CColliderComponent>(shape);
					obj->SetComponent(boxCollider);
					CPhysicsManager::GetInstance().SetCollider(boxCollider);

					auto debugMesh = std::make_shared<CMeshComponent>();
					obj->SetComponent(debugMesh);
					std::shared_ptr<CMesh> meshss = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, children->mesh.bounds.Extents);
					debugMesh->SetMesh(meshss);
				}

				if (children->name == "Table") {
					// 4) ColliderComponent 생성
					std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents);
					auto boxCollider = std::make_shared<CColliderComponent>(shape);
					obj->SetComponent(boxCollider);
					CPhysicsManager::GetInstance().SetCollider(boxCollider);

					auto debugMesh = std::make_shared<CMeshComponent>();
					obj->SetComponent(debugMesh);
					std::shared_ptr<CMesh> meshss = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, children->mesh.bounds.Extents);
					debugMesh->SetMesh(meshss);
				}

				obj->Initialize(GET_DEVICE, GET_CMD_LIST);

				objects.push_back(std::move(obj));
			}
		}

		// 카메라 객체 생성
		if (!camera) {
			camera = std::make_shared<CCamera>();
			camera->SetTarget(my_player.get());
			camera->Initialize(GET_DEVICE, GET_CMD_LIST);
		}

		// light 생성
		if (!light) {
			light = std::make_unique<CLightManager>();
			light->Initialize(GET_DEVICE, GET_CMD_LIST);
		}
	}
	else {
		std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>();
		otherPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);
		otherPlayer->SetID(pkt.info.player_id);
		otherPlayer->SetRoomID(pkt.room_id);
		otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
		otherPlayer->SetState(pkt.info.state);
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		EnterScene(otherPlayer, otherPlayer->GetID());
	}
}

void CScene::Handle_S_PLAYER_LIST(S_PLAYER_LIST& pkt)
{
	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저의 Player 생성
		std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>();
		otherPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);

		// 다른 유저의 Player ID 부여
		otherPlayer->SetID(userList[i].info.player_id);

		// 다른 유저의 Player 위치 부여
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// 다른 유저의 Player 상태 부여
		otherPlayer->SetState(userList[i].info.state);

		// 다른 유저의 Player 속한 씬 설정
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		// 굳이이긴 하나, 그래도 그냥 set 한다.
		otherPlayer->SetRoomID(pkt.room_id);

		// Active Scene에 다른 유저 입장
		EnterScene(otherPlayer, otherPlayer->GetID());
	}
}

void CScene::Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_Move& pkt)
{
	auto& vec = GetObjects();
	auto& indexMap = GetIDIndex();
	std::shared_ptr<CMyPlayer> myPlayer =GetMyPlayer();

	// 내 플레이어이면, 내 플레이어 보정용 함수 호출
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.player_id) {
		myPlayer->SetVelocity(pkt.info.vx, pkt.info.vy, pkt.info.vz);

		// 서버가 처리한 시퀀스 넘버를 받아야한다.
		myPlayer->ReconcileFromServer(pkt.last_seq_num, XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

		// 여기서 S_Move 패킷의 지터값 측정
		float now = CNetworkClockManager::GetInstance().GetClientNow();
		CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(now);
	}
	// 다른 플레이어일 경우
	else {
		// 해당 ID가 존재하는 플레이어인지 확인
		auto it = indexMap.find(pkt.info.player_id);
		if (it == indexMap.end())
			return;

		uint64 idx = it->second;
		if (idx >= vec.size())
			return;

		auto otherPlayer = std::static_pointer_cast<CPlayer>(vec[idx]);
		otherPlayer->SetYaw(pkt.info.yaw);
		otherPlayer->SetPitch(pkt.info.pitch);
		otherPlayer->SetState(pkt.info.state);

		// 회전을 위해 남겨둠
		{
			PlayerInfo info;
			info.yaw = pkt.info.yaw;
			info.pitch = pkt.info.pitch;
			info.roll = pkt.info.roll;
			otherPlayer->SetDestInfo(info);
		}

		// 상대 캐릭터는 서버 타임스탬프 기반 엔티티 보간 
		OpponentFrameHistory state{};
		state.player_id = pkt.info.player_id;
		state.state = pkt.info.state;
		state.position = XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z);
		state.server_timestamp = pkt.timestamp;

#ifdef GENERATE_LAG
		// 렉 시뮬레이터 작동
		CNetworkManager::GetInstance().OnRecvOpponentPos(state);
#else
		otherPlayer->RecordOpponentFrameHistory(state);

		// 여기서 S_Move 패킷의 지터값 측정
		float now = CNetworkClockManager::GetInstance().GetClientNow();
		CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(now);
#endif
	}
}
