#include "stdafx.h"
#include "Camera.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "Scene.h"
#include "ObjectFactory.h"
#include "ItemFactory.h"
#include "Item.h"
#include "Inventory.h"
#include "PhysicsManager.h"
#include "Collider.h"

#include "GameFramework.h"
#include "ServerSession.h"
#include "User.h"
#include "NetworkClockManager.h"
#include "ImGuiManager.h"
#include "Monster.h"
#include "GameScene.h"

#include "Animator.h"
#include "UIComponent.h"
#include "Renderers.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "ShadowMap.h"
#include "SkyBox.h"

CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
{
	ui_manager = std::make_shared<CUIManager>();
	scene_bounds.Center = XMFLOAT3{ 0, 0.0f, 0 };
	scene_bounds.Radius = 10;
}

CScene::~CScene()
{

}

void CScene::Initialize()
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
		if (obj->GetObjectType() != OBJECT_TYPE::STATIC_OBJECT)
			obj->Update(elapsedTime);
	}

	// 오브젝트 삭제
	std::vector<uint64> toDelete;
	for (const auto& obj : objects) {
		if (obj->IsPendingDelete())
			toDelete.push_back(obj->GetID());
	}

	for (uint64 id : toDelete)
		RemoveObject(id);
}

void CScene::Update(float elapsedTime)
{
	CPhysicsManager::GetInstance().Update(elapsedTime);
	AnimateObjects(elapsedTime);

	if (camera)
		camera->Update(my_player->position, elapsedTime);
	if (light)
		light->Update(camera.get(), scene_bounds);

	ui_manager->Update(elapsedTime);
}

void CScene::RenderShadowPass(ID3D12GraphicsCommandList* commandList)
{
	auto& shadowMap = CSceneManager::GetInstance().GetShadowMap();
	auto& cubeShadowMap = CSceneManager::GetInstance().GetCubeShadowMap();
	if (!shadowMap || !light || !cubeShadowMap) return;

	auto& shaders = CSceneManager::GetInstance().GetShaders();
	auto& renderers = CSceneManager::GetInstance().GetRanderers();

	// [Directional Light Shadow Map]
	shaders[EShaderName::Shadow]->RenderBegin(commandList);
	shadowMap->RenderBegin(commandList);
	light->Render(commandList);
	renderers[EShaderName::Shadow]->Render(commandList);
	shadowMap->RenderEnd(commandList);
	shaders[EShaderName::Shadow]->RenderEnd(commandList);

	// [Cube Shadow Map]
	shaders[EShaderName::CubeShadow]->RenderBegin(commandList);
	cubeShadowMap->RenderBegin(commandList);
	light->Render(commandList);
	BoundingFrustum cameraFrustum;
	if (camera) {
		cameraFrustum = camera->GetFrustum();
	}
	for (UINT i = 0; i < light->GetActiveDotNum(); ++i) {
		if (!light->IsPointLightVisible(i, cameraFrustum)) {
			continue;
		}
		// 화면에 보이는 조명일 때만 6개 축 큐브맵 섀도우 드로우 콜 발행
		commandList->SetGraphicsRoot32BitConstant(2, i, 0); // gCurrentLightIndex 세팅
		renderers[EShaderName::Shadow]->Render(commandList);
	}
	cubeShadowMap->RenderEnd(commandList);
	shaders[EShaderName::CubeShadow]->RenderEnd(commandList);

	renderers[EShaderName::Shadow]->ClearAllBatch();
}

void CScene::RenderSSAOPass(ID3D12GraphicsCommandList* commandList)
{
	auto& aoBuffer = CSceneManager::GetInstance().GetAOBuffer();
	if (!aoBuffer) return;

	auto& shaders = CSceneManager::GetInstance().GetShaders();
	aoBuffer->RenderBegin(commandList);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = aoBuffer->GetRTV();

	commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	const float clearColor[4] = { 1.f, 1.f, 1.f, 1.f };

	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	shaders[EShaderName::SSAO]->RenderBegin(commandList);

	camera->UpdateShaderVariablesShadow(commandList);
	camera->UpdateShaderVariables(commandList, false);
	commandList->DrawInstanced(3, 1, 0, 0);
	shaders[EShaderName::SSAO]->RenderEnd(commandList);
	aoBuffer->RenderEnd(commandList);
}

void CScene::RenderSSAOBlurPass(ID3D12GraphicsCommandList* commandList)
{
	auto& aoBuffer = CSceneManager::GetInstance().GetAOBuffer();          // SSAO 원본 결과물 (RT A)
	auto& aoBlurTemp = CSceneManager::GetInstance().GetAOBlurTemp();      // 가로 블러 임시 타겟 (RT B)
	if (!aoBuffer || !aoBlurTemp) return;
	auto& shaders = CSceneManager::GetInstance().GetShaders();

	auto ssaoBlurShader = shaders[EShaderName::SSAOBlur];
	auto ssaoBlurHeap = ssaoBlurShader->GetHeapManager();

	ssaoBlurShader->RenderBegin(commandList);

	const float clearColor[4] = { 1.f, 1.f, 1.f, 1.f };
	int width{ GET_CLIENT_WIDTH }, height{GET_CLIENT_HEIGHT};
	float texelWidth = 1.0f / width;
	float texelHeight = 1.0f / height;
	// PASS 1: 가로 블러 (Horizontal Blur)
	{
		aoBlurTemp->RenderBegin(commandList); // 내부에서 자원 상태를 RTV로 전환

		// 렌더 타겟 설정 (임시 버퍼 B)
		D3D12_CPU_DESCRIPTOR_HANDLE rtvTemp = aoBlurTemp->GetRTV();
		commandList->OMSetRenderTargets(1, &rtvTemp, FALSE, nullptr);
		commandList->ClearRenderTargetView(rtvTemp, clearColor, 0, nullptr);

		camera->UpdateShaderVariablesBlur(commandList, XMFLOAT2(1.0f, 0.0f), width, height, 0);

		// 드로우
		commandList->DrawInstanced(3, 1, 0, 0);
		aoBlurTemp->RenderEnd(commandList);
	}

	// PASS 2: 세로 블러 (Vertical Blur)
	{
		aoBuffer->RenderBegin(commandList);

		// 렌더 타겟 설정 (최종 AO 맵)
		D3D12_CPU_DESCRIPTOR_HANDLE rtvFinal = aoBuffer->GetRTV();
		commandList->OMSetRenderTargets(1, &rtvFinal, FALSE, nullptr);

		camera->UpdateShaderVariablesBlur(commandList, XMFLOAT2(0.0f, 1.0f), width, height, 1);

		// 최종 드로우
		commandList->DrawInstanced(3, 1, 0, 0);
		aoBuffer->RenderEnd(commandList);
	}

	ssaoBlurShader->RenderEnd(commandList);
}

void CScene::RenderBasePass(ID3D12GraphicsCommandList* commandList)
{
	if (camera) {
		camera->SetViewportsAndScissorRects(commandList);
	}

	auto& shaders = CSceneManager::GetInstance().GetShaders();
	auto& renderers = CSceneManager::GetInstance().GetRanderers();

	SetGBufferRenderTargets(commandList);
	for (size_t i = EShaderName::Skinning; i <= EShaderName::TwoSide; ++i) {
		if (!shaders[i]) continue;

		shaders[i]->RenderBegin(commandList);
		camera->UpdateShaderVariables(commandList, false);

		if (renderers[i]) {
			renderers[i]->Render(commandList);
			renderers[i]->ClearAllBatch();
		}
		shaders[i]->RenderEnd(commandList);
	}
}

void CScene::RenderDeferred(ID3D12GraphicsCommandList* commandList, ID3D12Resource* depthStencilBuf)
{
	auto& shaders = CSceneManager::GetInstance().GetShaders();
	auto& renderers = CSceneManager::GetInstance().GetRanderers();
	if (!shaders[EShaderName::Deferred] || !camera) return;

	TransitionGBuffersToSRV(commandList);
	TransitionDepthBuffer(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
	SetBackBufferRenderTarget(commandList);

	shaders[EShaderName::Deferred]->RenderBegin(commandList);

	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->IASetIndexBuffer(nullptr);

	camera->UpdateShaderVariables(commandList, false);
	camera->UpdateShaderVariablesShadow(commandList);
	if (light) {
		light->Render(commandList);
	}

	commandList->DrawInstanced(3, 1, 0, 0);
	shaders[EShaderName::Deferred]->RenderEnd(commandList);

	SetBackBufferWithDepthReadOnly(commandList);
	if (shaders[EShaderName::SkyBox]) {
		shaders[EShaderName::SkyBox]->RenderBegin(commandList);
		camera->UpdateShaderVariables(commandList, false);
		if (CSceneManager::GetInstance().GetSkybox()) {
			CSceneManager::GetInstance().GetSkybox()->Render(commandList, shaders[EShaderName::SkyBox]->GetHeapManager());
		}
		shaders[EShaderName::SkyBox]->RenderEnd(commandList);
	}
	if (shaders[EShaderName::Billboard]) {
		shaders[EShaderName::Billboard]->RenderBegin(commandList);
		camera->UpdateShaderVariablesBillBoard(commandList);
		renderers[EShaderName::Billboard]->Render(commandList);
		renderers[EShaderName::Billboard]->ClearAllBatch();
		shaders[EShaderName::Billboard]->RenderEnd(commandList);
	}
	if (shaders[EShaderName::UI] && renderers[EShaderName::UI]) {
		shaders[EShaderName::UI]->RenderBegin(commandList);
		camera->UpdateShaderVariables(commandList, true);

		renderers[EShaderName::UI]->Render(commandList);
		renderers[EShaderName::UI]->ClearAllBatch();

		renderers[EShaderName::Text]->Render(commandList);
		renderers[EShaderName::Text]->ClearAllBatch();

		shaders[EShaderName::UI]->RenderEnd(commandList);
	}

	TransitionDepthBuffer(commandList, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	RenderShadowPass(commandList);        // 1. 각 조명 시점 깊이 맵 빌드
	RenderBasePass(commandList);          // 2. 가시 물체 렌더링 및 G-Buffer 축적 (메인 뎁스는 DEPTH_WRITE 유지)
	RenderSSAOPass(commandList);
	RenderSSAOBlurPass(commandList);
}

void CScene::TransitionDepthBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = gGameFramework.GetDepthStencilBuffer().Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);
}

D3D12_RESOURCE_BARRIER CScene::CreateResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return barrier;
}

void CScene::SetGBufferRenderTargets(ID3D12GraphicsCommandList* commandList)
{
	CSceneManager& sceneManager = CSceneManager::GetInstance();
	
	D3D12_RESOURCE_BARRIER barriers[3] = {};
	barriers[0] = CreateResourceBarrier(sceneManager.GetGBufferColorResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	barriers[1] = CreateResourceBarrier(sceneManager.GetGBufferNormalResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	barriers[2] = CreateResourceBarrier(sceneManager.GetEmissiveResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(3, barriers);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3] = {
		sceneManager.GetGBufferColorRTV(),
		sceneManager.GetGBufferNormalRTV(),
		sceneManager.GetEmissiveRTV()
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = gGameFramework.GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(3, rtvHandles, FALSE, &dsvHandle);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandles[0], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(rtvHandles[1], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(rtvHandles[2], clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void CScene::TransitionGBuffersToSRV(ID3D12GraphicsCommandList* commandList)
{
	CSceneManager& sceneManager = CSceneManager::GetInstance();

	D3D12_RESOURCE_BARRIER barriers[3] = {};
	barriers[0] = CreateResourceBarrier(sceneManager.GetGBufferColorResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	barriers[1] = CreateResourceBarrier(sceneManager.GetGBufferNormalResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	barriers[2] = CreateResourceBarrier(sceneManager.GetEmissiveResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(3, barriers);
}

void CScene::SetBackBufferRenderTarget(ID3D12GraphicsCommandList* commandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv = gGameFramework.GetRtvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	backBufferRtv.ptr += (gGameFramework.GetSwapChainBufferIndex() * gGameFramework.GetRtvIncrementSize());

	commandList->OMSetRenderTargets(1, &backBufferRtv, FALSE, nullptr);
}

void CScene::SetBackBufferWithDepthReadOnly(ID3D12GraphicsCommandList* commandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv = gGameFramework.GetRtvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	backBufferRtv.ptr += (gGameFramework.GetSwapChainBufferIndex() * gGameFramework.GetRtvIncrementSize());

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = gGameFramework.GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(1, &backBufferRtv, FALSE, &dsvHandle);
}

void CScene::CollectObjects(ID3D12GraphicsCommandList* commandList)
{
	BoundingFrustum frustum;
	if (camera) {
		frustum = camera->GetFrustum();
	}

	auto& renderers = CSceneManager::GetInstance().GetRanderers();
	for (const auto& obj : objects) {
		if (obj->IsVisible(frustum)) {
			obj->OnCollect(renderers);
		}
	}

	if (my_player) {
		my_player->OnCollect(renderers);
	}

	ui_manager->Collect(renderers);
}

void CScene::Exit()
{
	CPhysicsManager::GetInstance().ClearCollider();
	RemoveAllMonsters();
	last_input_state = !last_input_state;
}

void CScene::Enter()
{
	for (const auto& obj : objects) {
		CColliderComponent* col = obj->GetComponent<CColliderComponent>();
		if (col) {
			CPhysicsManager::GetInstance().SetCollider(col);
		}
	}
}

void CScene::DrawUI_Final()
{
	ManageIME();
	DrawUI();

	if (!ImGui::IsAnyItemHovered())
		last_hovered_id_ = 0;
}

void CScene::ManageIME()
{
	// =======================
	// [IME 및 포커스 관리 로직]
	// =======================
	bool currentInputState = IsUIInputEnabled();
	HWND hwnd = ghWnd;

	// 상태 변경 시에만 IME 제어
	if (currentInputState != last_input_state || CImGuiManager::need_reset_focus) {

		if (currentInputState) {
			CImGuiManager::EnableIME(hwnd);
		}
		else {
			CImGuiManager::DisableIME(hwnd);
		}

		if (CImGuiManager::need_reset_focus) {
			ImGuiContext& g = *GImGui;
			g.ActiveId = 0;
			ImGui::SetWindowFocus(nullptr);
			ImGui::GetIO().InputQueueCharacters.resize(0);
			ImGui::GetIO().ClearInputKeys();
			CImGuiManager::ResetIMEState(hwnd);

			CImGuiManager::need_reset_focus = false;
		}

		last_input_state = currentInputState;
	}
}

void CScene::CheckHoverSound()
{
	if (!ImGui::IsItemHovered())
		return;

	ImGuiID id = ImGui::GetItemID();
	if (id != last_hovered_id_) {
		last_hovered_id_ = id;
		CSoundManager::GetInstance().Play(SOUND_ID::button01a);
	}
}

void CScene::PlayClickSound()
{
	CSoundManager::GetInstance().Play(SOUND_ID::select09);
}

void CScene::AddObject(std::shared_ptr<CObject> obj, UINT id)
{
	id_To_Index[id] = objects.size();
	objects.push_back(obj);
}

void CScene::RemoveObject(UINT id)
{
	auto iter = id_To_Index.find(id);
	if (iter == id_To_Index.end())
		return;

	UINT idx = iter->second;

	if (idx >= objects.size() || !objects[idx]) {
		id_To_Index.erase(id);
		return;
	}

	UINT last = (UINT)objects.size() - 1;

	if (auto* col = objects[idx]->GetComponent<CColliderComponent>())
		CPhysicsManager::GetInstance().EraseCollider(col);

	std::swap(objects[idx], objects[last]);
	if (objects[idx]) {
		id_To_Index[objects[idx]->GetID()] = idx;
	}

	objects.pop_back();
	id_To_Index.erase(id);
}

void CScene::RemoveAllMonsters()
{
	std::vector<UINT> eraseIds;

	for (const auto& [id, idx] : id_To_Index) {
		if (id >= 1001)
			eraseIds.push_back(id);
	}

	for (UINT id : eraseIds) {
		RemoveObject(id);
	}
}

void CScene::Handle_S_Spawn_Player(std::shared_ptr<Session>& session, const S_SpawnPlayer& pkt)
{
	auto shaders = CSceneManager::GetInstance().GetShaders();
	auto& factory = CSceneManager::GetInstance().GetFactory();

	if (pkt.is_my_player) {
		{
			my_player = factory->CreateMyPlayer();

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
	}
	else {
		std::shared_ptr<CPlayer> otherPlayer = factory->CreatePlayer();
		otherPlayer->SetID(pkt.info.player_id);
		otherPlayer->SetRoomID(pkt.room_id);
		otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
		otherPlayer->SetState(pkt.info.state);
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		otherPlayer->ChangeModelSet(pkt.info.body_type);
		otherPlayer->ChangeEyes(pkt.info.eyes_type);
		otherPlayer->ChangeMouth(pkt.info.mouth_type);
		factory->UpdatePlayerTextures(otherPlayer);

		AddObject(otherPlayer, otherPlayer->GetID());
		player_slot_ids.push_back(otherPlayer->GetID());
	}
}

void CScene::Handle_S_PLAYER_LIST(S_PLAYER_LIST& pkt)
{
	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	auto shaders = CSceneManager::GetInstance().GetShaders();
	auto& factory = CSceneManager::GetInstance().GetFactory();
	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저의 Player 생성
		std::shared_ptr<CPlayer> otherPlayer = factory->CreatePlayer();

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

		// 다른 플레이어의 캐릭터 커스터마이즈
		otherPlayer->ChangeModelSet(userList[i].info.body_type);
		otherPlayer->ChangeEyes(userList[i].info.eyes_type);
		otherPlayer->ChangeMouth(userList[i].info.mouth_type);
		factory->UpdatePlayerTextures(otherPlayer);

		// Active Scene에 다른 유저 입장
		AddObject(otherPlayer, otherPlayer->GetID());

		player_slot_ids.push_back(otherPlayer->GetID());
	}
}

void CScene::Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_PlayerMove& pkt)
{
	auto& vec = GetObjects();
	auto& indexMap = GetIDIndex();
	std::shared_ptr<CMyPlayer> myPlayer = GetMyPlayer();

	// 내 플레이어이면, 내 플레이어 보정용 함수 호출
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.player_id) {

		myPlayer->SetServerVelocity(XMFLOAT3{pkt.info.vx, pkt.info.vy, pkt.info.vz});
		PLAYER_STATE prevState = myPlayer->GetState();
		myPlayer->SetState(pkt.info.state);

		if (prevState != PLAYER_STATE::ATTACK && pkt.info.state == PLAYER_STATE::ATTACK)
			myPlayer->OnAttack();

		myPlayer->SetIsGrounded((pkt.info.state != PLAYER_STATE::JUMP));
		myPlayer->SetStaminaFromServer(pkt.stamina);
		myPlayer->SetHp(pkt.hp);

		bool prevPossessed = myPlayer->GetIsPossessed();
		bool prevStunned   = myPlayer->GetIsStunned();

		myPlayer->SetPossessed(pkt.info.is_possessed);
		myPlayer->SetStunned(pkt.info.is_stunned);

		if (prevPossessed != pkt.info.is_possessed || prevStunned != pkt.info.is_stunned ||
			prevState != pkt.info.state) {
			auto& factory = CSceneManager::GetInstance().GetFactory();
			if (factory) 
				factory->UpdateEyeTexture(myPlayer);
		}

		// 라운드 타이머 동기화 (GameScene일 때만 유효한 값이 들어옴)
		if (pkt.round_timer >= 0.f) {
			CScene* active = CSceneManager::GetInstance().GetActiveScene();
			if (active && active->GetSceneType() == SCENE_TYPE::GAME) {
				static_cast<CGameScene*>(active)->SetRoundTimer(pkt.round_timer);
			}
		}

		// 예측 이동을 없애고
		// 아래의 코드가 추가되었다.
		{
			PlayerInfo info;
			info.x = pkt.info.x;
			info.y = pkt.info.y;
			info.z = pkt.info.z;
			myPlayer->SetDestInfo(info);
		}

		// 서버가 처리한 시퀀스 넘버를 받아야한다.
		//myPlayer->ReconcileFromServer(pkt.last_seq_num, XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

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

		PLAYER_STATE prevNetState = otherPlayer->GetLastNetState();
		otherPlayer->SetLastNetState(pkt.info.state);
		if (prevNetState != PLAYER_STATE::ATTACK && pkt.info.state == PLAYER_STATE::ATTACK) {
			auto* animator = otherPlayer->GetComponent<CAnimatorComponent>();
			if (animator) 
				animator->PlayAction(animator->GetAttackClipByItem(otherPlayer->GetEquippedItemId()));
		}

		bool prevOtherPossessed = otherPlayer->GetIsPossessed();
		bool prevOtherStunned   = otherPlayer->GetIsStunned();

		PLAYER_STATE prevOtherState = otherPlayer->GetState();
		otherPlayer->SetState(pkt.info.state);
		otherPlayer->SetHp(pkt.hp);
		otherPlayer->SetPossessed(pkt.info.is_possessed);
		otherPlayer->SetStunned(pkt.info.is_stunned);

		if (prevOtherPossessed != pkt.info.is_possessed || prevOtherStunned != pkt.info.is_stunned ||
			prevOtherState != pkt.info.state) {
			auto& factory = CSceneManager::GetInstance().GetFactory();
			if (factory) 
				factory->UpdateEyeTexture(otherPlayer);
		}

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

void CScene::Handle_S_Remove_Player(std::shared_ptr<Session>& session, const S_RemovePlayer& pkt)
{
	RemoveObject(pkt.player_id);
}

void CScene::Handle_S_Spawn_Monster(std::shared_ptr<Session>& session, const S_SpawnMonster& pkt)
{
	uint32 monsterId = pkt.info.monster_id;
	MON_TYPE type = pkt.info.monster_type;
	NetMonsterInfo info = pkt.info;
	XMFLOAT3 pos{ pkt.info.x, pkt.info.y, pkt.info.z };
	
	auto& factory = CSceneManager::GetInstance().GetFactory();
	auto monster = factory->CreateMonster(type, scene_type);
	monster->SetID(monsterId);
	monster->SetPosition(pos);
	monster->SetOriginPos(pos);
	AddObject(monster, monsterId);
}

void CScene::Handle_S_DeSpawn_Monster(std::shared_ptr<Session>& session, const S_DeSpawnMonster& pkt)
{
	auto& indexMap = GetIDIndex();
	auto it = indexMap.find(pkt.monster_id);
	if (it == indexMap.end())
		return;

	objects[it->second]->MarkForDelete();
}

void CScene::Handle_S_Move_Monster(std::shared_ptr<Session>& session, const S_MonsterMove& pkt)
{
	auto& vec = GetObjects();
	auto& indexMap = GetIDIndex();

	// 해당 ID가 존재하는 몬스터인지 확인
	auto it = indexMap.find(pkt.info.monster_id);
	if (it == indexMap.end())
		return;

	uint64 idx = it->second;
	if (idx >= vec.size())
		return;

	auto monster = std::static_pointer_cast<CMonster>(vec[idx]);

	// 상대 캐릭터는 서버 타임스탬프 기반 엔티티 보간
	MonsterFrameHistory state{};
	state.monster_id = pkt.info.monster_id;
	state.AI_state = pkt.info.AI_state;
	state.position = XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z);
	state.yaw = pkt.info.yaw;
	state.pitch = pkt.info.pitch;
	state.server_timestamp = pkt.timestamp;

	monster->RecordMonsterFrameHistory(state);
}

void CScene::Handle_S_Scene_Change(std::shared_ptr<Session>& session, const S_SceneChange& pkt)
{
	CSceneManager::GetInstance().ChangeScene(pkt.target_scene);
}

void CScene::Handle_S_AddItemList(std::shared_ptr<Session> session, S_AddItemList& pkt)
{
	// 여러 아이템을 도감번호로 생성해 인벤토리에 추가한다.
	// 줍기(월드 아이템)와 달리 월드 오브젝트가 없어도 되므로 로비(상점 구매)/게임 양쪽에서 사용한다.
	S_AddItemList::ItemList itemList = pkt.GetItemList();

	for (uint32 i = 0; i < pkt.item_count; ++i) {
		auto item = ItemFactory::Create(itemList[i].item_id);
		if (!item)
			continue;

		if (!my_player) {
			// 씬 전환 타이밍상 my_player가 아직 이 씬에 없으면 로비씬의 것을 사용
			my_player = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY]->GetMyPlayer();
		}

		// 내구도 복원 (장비이고 내구도 값이 유효할 때만). 픽업(S_AddItem)과 동일 패턴.
		if (itemList[i].durability > 0) {
			if (auto equip = std::dynamic_pointer_cast<CEquipment>(item))
				equip->SetCurrentDurability((uint16)itemList[i].durability);
		}

		if (my_player && my_player->GetInventory())
			my_player->GetInventory()->AddItemWithId(item, itemList[i].inventory_id);
	}
}

void CScene::Handle_S_UpdateCoin(std::shared_ptr<Session> session, S_UpdateCoin& pkt)
{
	if (my_player->GetID() != pkt.player_id)
		return;

	my_player->SetGold(pkt.coin);
}
