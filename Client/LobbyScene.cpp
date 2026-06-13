#include "stdafx.h"
#include "LobbyScene.h"
#include "CustomScene.h"
#include "SceneManager.h"
#include "KeyManager.h"
#include "ImGuiManager.h"

#include "MyPlayer.h"
#include "Inventory.h"
#include "Camera.h"
#include "Shader.h"
#include "ObjectFactory.h"
#include "UIComponent.h"
#include "DataManager.h"

#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "GameFramework.h"
#include "Shop.h"

CLobbyScene::CLobbyScene()
	: CScene(SCENE_TYPE::LOBBY)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::Initialize()
{
	CScene::Initialize();
	auto& factory = CSceneManager::GetInstance().GetFactory();

	if (objects.empty()) {
		objects = factory->CreateLobby();
		objects.push_back(std::make_shared<CParticleObject>(XMFLOAT3{ -2.5f, 0.17f, -0.216999993f }));
	}

	// UI 생성
	auto mainCanvas = ui_manager->CreateCanvas();
	auto repeaper = factory->CreateReaper();

	objects.push_back(repeaper);

	// LoadData
	ui_manager->GetDataManager()->LoadScripts("../Modeling/UI/Reaper.json");
	// ESC 메뉴는 ImGui(DrawMenu)로 직접 그림 — Menu_UI.json 로드하지 않음
	// 대사 UI
	auto reaperCanvas = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/ReaperDialogue.json");
	reaperCanvas->SetEnable(false);
	ui_manager->AddCanvas(reaperCanvas);
	// Ready UI
	auto ReadyCanvas = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/PlayerReady.json");
	ui_manager->AddCanvas(ReadyCanvas);
	// Player UI
	auto playerUI = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/Player_UI.json");
	ui_manager->AddCanvas(playerUI);
	// player data와 연동
	auto hpBar = ui_manager->GetUI<CUIImage>("HP_UI");
	// 0 ~ 1 사이 값으로 변환
	hpBar->BindFillAmount([this]() -> float {
		if (!my_player) return 0.0f;
		float current = static_cast<float>(my_player->GetHp());
		float max = static_cast<float>(my_player->GetMaxHp());
		return current / max;
		});
	auto energyBar = ui_manager->GetUI<CUIImage>("ENERGY_UI");
	energyBar->BindFillAmount([this]() -> float {
		if (!my_player) return 0.0f;
		float current = static_cast<float>(my_player->GetStamina());
		float max = static_cast<float>(my_player->GetMaxStamina());
		return current / max;
		});

	SetButtonEvents();
}

void CLobbyScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		auto& factory = CSceneManager::GetInstance().GetFactory();
		my_player = factory->CreateMyPlayer();
	}

	if (!camera) {
		camera = std::make_shared<CCamera>();
		camera->SetTarget(my_player.get());
		camera->Initialize(device, commandList);
	}
	
	// light 생성
	if (!light) {
		light = std::make_unique<CLightManager>();
		light->Initialize(device, commandList);
		light->AddPointLight(XMFLOAT3(2.49913216f, 1.5f, 1.45683444f), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 2.0f);			// Lamp on table
		light->AddPointLight(XMFLOAT3(1.83547652f, 2.55953884f, -2.89159632f), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 2.0f);	// Lamp on StoneBed
		light->AddPointLight(XMFLOAT3(-2.58500004f, 1.03499997f, 4.06699991f), XMFLOAT3(0.5f, 0.5f, 0.5f), 0.5f, 1.5f);	// reaper
		light->AddPointLight(XMFLOAT3(-2.44799995f, 0.48300001f, -0.216999993f), XMFLOAT3(1.0f, 0.8f, 0.6f), 0.5f, 1.5f);	// firewood
		light->AddPointLight(XMFLOAT3(0.0309999995f, 1.972f, 4.13100004f), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 3.0f);		// pipe
	}
}

void CLobbyScene::Update(float elapsedTime)
{
	// ESC 메뉴가 열려 있으면 일시정지: 씬 업데이트/입력 전송 차단. ESC로 닫기만 처리.
	if (paused) {
		if (KEY_TAP(KEY::ESC)) {
			ui_state = LobbyUIState::None;
			paused   = false;
			CKeyManager::GetInstance().SetMouseMode(true);   // 게임 커서 모드 복귀
		}
		return;
	}

	CScene::Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
		UpdatePlayerReadyUI();
	}


	bool isInvOpen = (my_player && my_player->GetInventory() && my_player->GetInventory()->IsOpen());
	if (KEY_TAP(KEY::ESC)) {
		// 1순위: 상점이 열려 있으면 상점 닫기
		if (CShop::GetInstance().IsOpen()) {
			CShop::GetInstance().Close();
		}
		// 2순위: 대화창이 열려 있으면 대화창 닫기
		else if (auto reaperUI = ui_manager->GetUI<CUICanvas>("ReaperSpeechCanvas"); reaperUI && reaperUI->is_enable) {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
		}
		// 3순위: 인벤토리가 닫혀 있을 때만 ESC 메뉴를 켬 (일시정지)
		else if (!isInvOpen) {
			ui_state = LobbyUIState::Menu;
			paused   = true;
			CKeyManager::GetInstance().SetMouseMode(false);  // UI 커서 모드
		}
	}

	// C 키로 위치 기반 상호작용 (인벤토리가 닫혀 있을 때만 작동 가능)
	if (KEY_TAP(KEY::C) && my_player && !CShop::GetInstance().IsOpen() && !isInvOpen) {
		switch (GetInteractZone()) {
		case InteractZone::Entrance:
			InteractWithReaper();
			break;
		case InteractZone::Reaper:
			CShop::GetInstance().Open();
			break;
		default:
			break;
		}
	}

	// 상점 열림/닫힘 전환 처리
	HandleShopTransition();

	if (KEY_TAP(KEY::LBTN)) {
		auto reaperUI = ui_manager->FindUI<CUICanvas>("ReaperSpeechCanvas");
		if (reaperUI && reaperUI->is_enable) {
			auto reaperText = ui_manager->FindUI<CUIText>("ReaperText");
			if (reaperText) {
				reaperText->Skip();
				CKeyManager::GetInstance().SetMouseMode(false);
			}
		}
	}
}

void CLobbyScene::InteractWithReaper()
{
	// 찾은 대사를 UI 텍스트 컴포넌트에 전달
	ui_manager->ToggleUI("ReaperSpeechCanvas", true, true);
	auto text = ui_manager->GetUI<CUIText>("ReaperText");
	if (text)
		text->SetText(ui_manager->GetDataManager()->GetDialogue("Reaper", "Ask_Exit"));
}

CLobbyScene::InteractZone CLobbyScene::GetInteractZone() const
{
	if (!my_player)
		return InteractZone::None;

	// XZ 평면 제곱거리 비교, 가까운 앵커 선택
	XMFLOAT3 pos = my_player->GetPosition();
	float distReaperSq   = (pos.x - reaper_anchor.x)   * (pos.x - reaper_anchor.x)   + (pos.z - reaper_anchor.y)   * (pos.z - reaper_anchor.y);
	float distEntranceSq = (pos.x - entrance_anchor.x) * (pos.x - entrance_anchor.x) + (pos.z - entrance_anchor.y) * (pos.z - entrance_anchor.y);

	if (distEntranceSq <= interact_radius_sq && distEntranceSq <= distReaperSq)
		return InteractZone::Entrance;
	if (distReaperSq <= interact_radius_sq)
		return InteractZone::Reaper;

	return InteractZone::None;
}

void CLobbyScene::DrawInteractPrompt(InteractZone zone)
{
	if (zone == InteractZone::None)
		return;

	// 다이얼로그가 열려 있으면 안내를 숨겨 겹침 방지
	auto speechCanvas = ui_manager->FindUI<CUICanvas>("ReaperSpeechCanvas");
	if (speechCanvas && speechCanvas->is_enable)
		return;

	ImGuiIO&    io   = ImGui::GetIO();
	ImDrawList* dl   = ImGui::GetForegroundDrawList();
	ImFont*     font = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();
	const float scale = G_RATIO_Y;

	// Reaper=상점, Entrance=지상으로 떠나기
	const char* text = (zone == InteractZone::Reaper)
		? "[ C ] \xEC\x83\x81\xEC\xA0\x90 \xEC\x9D\xB4\xEC\x9A\xA9\xED\x95\x98\xEA\xB8\xB0"
		: "[ C ] \xEC\xA7\x80\xEC\x83\x81\xEC\x9C\xBC\xEB\xA1\x9C \xEB\x96\xa0\xEB\x82\x98\xEA\xB8\xB0";
	const float fontPx = 32.f * scale;

	ImVec2 textSize = font->CalcTextSizeA(fontPx, FLT_MAX, 0.f, text);
	ImVec2 pos      = ImVec2(io.DisplaySize.x * 0.5f - textSize.x * 0.5f, io.DisplaySize.y * 0.62f);

	// 외곽선(그림자) 4방향 + 본문 (사신=하늘색, 입구=노란빛)
	ImVec4 mainColor = (zone == InteractZone::Reaper)
		? ImVec4(0.5f, 0.85f, 1.f, 1.f)
		: ImVec4(1.f, 0.95f, 0.6f, 1.f);
	ImU32 shadow = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.7f));
	ImU32 main   = ImGui::GetColorU32(mainColor);
	const float off = 1.5f * scale;

	dl->AddText(font, fontPx, ImVec2(pos.x - off, pos.y), shadow, text);
	dl->AddText(font, fontPx, ImVec2(pos.x + off, pos.y), shadow, text);
	dl->AddText(font, fontPx, ImVec2(pos.x, pos.y - off), shadow, text);
	dl->AddText(font, fontPx, ImVec2(pos.x, pos.y + off), shadow, text);
	dl->AddText(font, fontPx, pos, main, text);
}

void CLobbyScene::HandleShopTransition()
{
	bool open = CShop::GetInstance().IsOpen();
	if (open == shop_was_open)
		return;

	auto inventory = my_player ? my_player->GetInventory() : nullptr;

	if (open) {
		// 상점 진입: 기존 인벤토리 상태 저장 후 강제로 열고 커서 모드 활성화
		if (inventory) {
			prev_inventory_open = inventory->IsOpen();
			inventory->SetOpen(true);

			if (g_is_single) {
				my_player->SetVelocity(0, 0, 0);
				my_player->SetState(PLAYER_STATE::IDLE);
				my_player->SetPosition(reaper_anchor.x, 0.1f, reaper_anchor.y);
			}
		}
		CKeyManager::GetInstance().SetMouseMode(false);   // false = UI 커서 모드

		// 멀티: 서버에 "상점 이용 중" 통보 → 서버가 플레이어 정지/고정
		if (!g_is_single && my_player) {
			C_ShopState pkt;
			pkt.player_id = my_player->GetID();
			if (auto session = my_player->GetSession()) {
				auto sendBuffer = MAKE_SEND_BUFFER(pkt);
				session->DoSend(sendBuffer);
			}
		}
	}
	else {
		// 상점 종료: 위치 오버라이드 해제 + 인벤토리 이전 상태 복원
		if (inventory) {
			inventory->ClearPositionOverride();
			inventory->SetOpen(prev_inventory_open);
		}
		CKeyManager::GetInstance().SetMouseMode(prev_inventory_open ? false : true);
	}

	shop_was_open = open;
}

void CLobbyScene::SetButtonEvents()
{
	auto reaperText = ui_manager->GetUI<CUIText>("ReaperText");
	auto yesBtn = ui_manager->GetUI<CUIButton>("YesButton");
	auto noBtn = ui_manager->GetUI<CUIButton>("NoButton");

	if (yesBtn) yesBtn->SetEnable(false);
	if (noBtn) noBtn->SetEnable(false);

	if (reaperText) {
		reaperText->onFinished = [yesBtn, noBtn]() {
			if (yesBtn) yesBtn->SetEnable(true);
			if (noBtn) noBtn->SetEnable(true);
			};
	}

	if (yesBtn) {
		yesBtn->OnClick = [this]() {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::GAME);
			}
			else {
				std::shared_ptr<CMyPlayer> myPlayer = this->GetMyPlayer();
				if (myPlayer && !myPlayer->GetIsReady()) {
					myPlayer->SetIsReady(true);
					C_Ready readyPkt;
					readyPkt.player_id = myPlayer->GetID();
					if (auto session = myPlayer->GetSession()) {
						auto sendBuffer = MAKE_SEND_BUFFER(readyPkt);
						session->DoSend(sendBuffer);
					}
				}
			}
			};
	}

	if (noBtn) {
		noBtn->OnClick = [this]() {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			};
	}
}

void CLobbyScene::UpdatePlayerReadyUI()
{
	if (!g_is_single) 
		return;

	auto SetReadyUIColor = [this](int playerIdx, bool isReady) {
		// 이름 규칙: "Ready1", "Ready2", "Ready3"...
		std::string uiName = "Ready" + std::to_string(playerIdx + 1);
		auto readyUI = ui_manager->FindUI<CUIImage>(uiName);

		if (readyUI) {
			XMVECTOR color = isReady ? XMVectorSet(0, 0, 1, 1) : XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f);
			XMFLOAT4 finalColor;
			XMStoreFloat4(&finalColor, color);
			readyUI->SetColor(finalColor);
		}
	};

	SetReadyUIColor(0, my_player->GetIsReady());
}

void CLobbyScene::Enter()
{
	CScene::Enter();

	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::LOBBY);
		camera->SetTarget(my_player.get());
		my_player->ResetAll();
		my_player->SetPosition(XMFLOAT3{ 0, 0, 0 });
	}

	for (int i = 1; i <= 4; ++i) {
		auto readyUI = ui_manager->GetUI<CUIImage>("Ready" + std::to_string(i));
		if (readyUI)
			readyUI->SetColor(XMFLOAT4{ 1, 0, 0, 1 });
	}

	// (테스트) 로비씬에 입장하면 딱 한번만 10000원 준다.
	static bool once = false;
	if (g_is_single && !once) {
		my_player->SetGold(100000000);
		once = true;
	}
}

void CLobbyScene::Exit()
{
	CScene::Exit();

	// 상점이 열린 채 씬 전환되는 경우 정리
	if (CShop::GetInstance().IsOpen())
		CShop::GetInstance().Close();
	if (my_player) {
		if (auto inv = my_player->GetInventory())
			inv->ClearPositionOverride();
	}

	shop_was_open = false;

	my_player = nullptr;
}

bool CLobbyScene::IsUIInputEnabled()
{
	bool state = true;

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene);

	if (scene->GetSceneType() == SCENE_TYPE::LOBBY)
		state = false;

	// 상점이 열려 있으면 ImGui 입력(클릭/버튼)을 허용해야 함
	if (CShop::GetInstance().IsOpen())
		state = true;

	return state;
}

void CLobbyScene::DrawUI()
{
	if (!my_player)
		return;

	if (CShop::GetInstance().IsOpen()) {
		// 상점: 좌 상점 패널 + 우 인벤토리를 함께 그림 (DrawStoreUI가 인벤토리도 그림)
		CShop::GetInstance().DrawStoreUI(my_player);
	}
	else {
		// 상호작용 구역 안이면 "Press C key" 안내 표시
		DrawInteractPrompt(GetInteractZone());

		// LobbyScene은 인벤토리 조회 전용 (드래그/드롭/퀵슬롯 등록 차단)
		auto inventory = my_player->GetInventory();
		if (inventory)
			inventory->Draw(true);
	}

	// ESC 메뉴 (최상위 오버레이)
	if (ui_state == LobbyUIState::Menu)
		DrawMenu();
}

void CLobbyScene::DrawMenu()
{
	ImVec2 screenSize = ImGui::GetIO().DisplaySize;
	float  scale      = G_RATIO_Y;

	// 배경 디밍: 화면 전체를 75% 불투명 어두운 회색으로 덮음 (블러 아님)
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		ImVec2(0, 0), screenSize, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.75f)));

	// 창을 화면 정중앙에 배치
	ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;

	if (ImGui::Begin("Lobby Menu", nullptr, menuFlags)) {
		ImGui::SetWindowFontScale(scale);

		ImVec2 btnSize = ImVec2(200.0f * scale, 60.0f * scale);

		ImGui::Spacing(); ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.5f, 0.5f, 0.5f, 1.0f));   // 평소
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));   // 호버
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.6f, 0.1f, 0.1f, 1.0f));   // 클릭

		// [커스텀] 버튼 → 커스텀 씬으로 이동
		if (ImGui::Button((const char*)u8"커스텀", btnSize)) {
			ui_state = LobbyUIState::None;
			paused   = false;

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
			}
			else {
				// 멀티: C_SceneChange만 송신, 서버의 S_SceneChange 응답을 기다림 (로컬 ChangeScene 금지)
				C_SceneChange changeScenePkt;
				changeScenePkt.player_id     = my_player->GetID();
				changeScenePkt.current_scene = scene_type;
				changeScenePkt.target_scene  = SCENE_TYPE::CUSTOMS;

				auto session = GET_SERVER_SESSION;
				assert(session);

				auto sendBuffer = MAKE_SEND_BUFFER(changeScenePkt);
				session->DoSend(sendBuffer);
			}
		}

		ImGui::Spacing(); ImGui::Spacing();

		// [나가기] 버튼 → 확인 팝업
		if (ImGui::Button((const char*)u8"나가기", btnSize)) {
			ImGui::OpenPopup((const char*)u8"LeaveConfirmPopup");
		}

		ImGui::Spacing(); ImGui::Spacing();
		ImGui::PopStyleColor(3);
		ImGui::SetWindowFontScale(1.0f);

		DrawRoomLeavePopUp();
	}
	ImGui::End();
}

void CLobbyScene::DrawRoomLeavePopUp()
{
	ImVec2 screenSize = ImGui::GetIO().DisplaySize;
	float  scale      = G_RATIO_Y;

	ImVec2 popupCenter = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f);
	ImGui::SetNextWindowPos(popupCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal((const char*)u8"LeaveConfirmPopup", NULL,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {

		ImGui::SetWindowFontScale(scale);

		ImGui::Text((const char*)u8"정말로 나가시겠습니까?");
		ImGui::Separator();
		ImGui::Spacing();

		ImVec2 popupBtnSize = ImVec2(100.0f * scale, 40.0f * scale);

		// [확인] → 방 나가고 타이틀로
		if (ImGui::Button((const char*)u8"확인", popupBtnSize)) {

			// 멀티일 때만 서버에 방 나가기 통보 (서버가 마지막 유저면 방 삭제)
			if (!g_is_single) {
				C_LeaveRoom leavePkt;
				leavePkt.user_id = my_player->GetID();

				auto session = GET_SERVER_SESSION;
				assert(session);

				auto sendBuffer = MAKE_SEND_BUFFER(leavePkt);
				session->DoSend(sendBuffer);
			}

			// 커스터마이징 인덱스 초기화
			CCustomScene* customScene = (CCustomScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get();
			customScene->body_idx  = 0;
			customScene->eyes_idx  = 0;
			customScene->mouth_idx = 0;

			ui_state = LobbyUIState::None;
			paused   = false;

			ImGui::CloseCurrentPopup();
			CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::TITLE);
		}

		ImGui::SameLine();

		// [취소] → 팝업만 닫기 (ESC 메뉴는 유지)
		if (ImGui::Button((const char*)u8"취소", popupBtnSize)) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::SetWindowFontScale(1.0f);
		ImGui::EndPopup();
	}
}

// 서버 패킷 처리 관련 함수들
void CLobbyScene::Handle_S_MapStart(std::shared_ptr<Session> session, const S_MapStart& pkt)
{
	//StartLoading(LoadingType::GenerateMap);
}

void CLobbyScene::Handle_S_Ready(std::shared_ptr<Session> session, const S_Ready& pkt)
{
	int slot = -1;

	if (my_player && pkt.player_id == my_player->GetID()) {
		slot = 1;  // 본인은 항상 Ready1
	}
	else {
		auto it = std::find(player_slot_ids.begin(), player_slot_ids.end(), pkt.player_id);
		if (it == player_slot_ids.end()) 
			return;

		slot = (int)std::distance(player_slot_ids.begin(), it) + 2;  // 1-based
		if (slot < 1 || slot > 4) 
			return;
	}

	auto readyUI = ui_manager->GetUI<CUIImage>("Ready" + std::to_string(slot));
	if (readyUI)
		readyUI->SetColor(XMFLOAT4{ 0, 0, 1, 1 });
}

void CLobbyScene::Handle_S_RefreshStore(std::shared_ptr<Session> session, const S_RefreshStore& pkt)
{
	if (my_player->GetID() != pkt.player_id)
		return;

	CShop::GetInstance().Reset();
}
