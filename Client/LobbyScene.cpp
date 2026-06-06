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
	}

	// UI 생성
	auto mainCanvas = ui_manager->CreateCanvas();
	auto repeaper = factory->CreateReaper();

	objects.push_back(repeaper);

	// LoadData
	ui_manager->GetDataManager()->LoadScripts("../Modeling/UI/Reaper.json");
	// menu UI
	auto menuUI = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/Menu_UI.json");
	menuUI->SetEnable(false);
	ui_manager->AddCanvas(menuUI);
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
		light->AddPointLight(XMFLOAT3(2.49913216, 1.08363628, 1.45683444), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 3.0f);
		light->AddPointLight(XMFLOAT3(1.83547652, 2.55953884, -2.89159632), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 3.0f);
	}
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
		UpdatePlayerReadyUI();
	}

	if (KEY_TAP(KEY::ESC)) {
		// 상점이 열려 있으면 ESC는 상점 닫기 (메뉴보다 우선)
		if (CShop::GetInstance().IsOpen()) {
			CShop::GetInstance().Close();
		}
		else {
			auto menuUI = ui_manager->GetUI<CUICanvas>("LobbyMenuCanvas");
			if (menuUI) {
				ui_manager->ToggleUI("LobbyMenuCanvas", !menuUI->is_enable, menuUI->is_enable);
			}
		}
	}

	// C 키로 위치 기반 상호작용 (상점이 열려 있을 땐 무시)
	if (KEY_TAP(KEY::C) && my_player && !CShop::GetInstance().IsOpen()) {
		switch (GetInteractZone()) {
		case InteractZone::Entrance:
			InteractWithReaper();   // Ground 입구 → 게임씬 진입 준비
			break;
		case InteractZone::Reaper:
			CShop::GetInstance().Open();   // 사신(상점) NPC → 상점 열기
			break;
		default:
			break;
		}
	}

	// 상점 열림/닫힘 전환 처리 (인벤토리 강제 열기/복원, 커서 모드)
	HandleShopTransition();

	if (KEY_PRESSED(KEY::LBTN)) {
		auto reaperUI = ui_manager->FindUI<CUICanvas>("ReaperSpeechCanvas");
		if (reaperUI->is_enable) {
			auto reaperText = ui_manager->FindUI<CUIText>("ReaperText");
			reaperText->Skip();	// 디버깅을 위해 스킵
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

	const char* text   = "Press C key";
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
	auto menuToCustomBtn = ui_manager->GetUI<CUIButton>("ToCustom");
	auto menuBackBtn = ui_manager->GetUI<CUIButton>("Back");

	if (yesBtn) yesBtn->SetEnable(false);
	if (noBtn) noBtn->SetEnable(false);

	if (reaperText) {
		reaperText->onFinished = [yesBtn, noBtn]() {
			bool isGameMode = CKeyManager::GetInstance().GetMouseMode();
			if (yesBtn) {
				yesBtn->SetEnable(true);
				if (isGameMode) {
					CKeyManager::GetInstance().SetMouseMode(false);
				}
			}
			if (noBtn) {
				noBtn->SetEnable(true);
				if (isGameMode) {
					CKeyManager::GetInstance().SetMouseMode(false);
				}
			}
			};
	}

	if (yesBtn) {
		yesBtn->OnClick = [this, yesBtn]() {
			bool isGameMode = CKeyManager::GetInstance().GetMouseMode();
			if (!isGameMode) {
				CKeyManager::GetInstance().SetMouseMode(true);
			}

			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::GAME);
			}
			else {
				std::shared_ptr<CMyPlayer>myPlayer = this->GetMyPlayer();
				if (!myPlayer->GetIsReady()) {
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
		noBtn->OnClick = [this, noBtn]() {
			bool isGameMode = CKeyManager::GetInstance().GetMouseMode();
			if (!isGameMode) {
				CKeyManager::GetInstance().SetMouseMode(true);
			}

			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			};
	}

	if (menuToCustomBtn) {
		menuToCustomBtn->OnClick = [this]() {
			ui_manager->ToggleUI("LobbyMenuCanvas", false, false);

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
			}
			else {
				C_SceneChange changeScenePkt;
				changeScenePkt.player_id = my_player->GetID();
				changeScenePkt.current_scene = scene_type;
				changeScenePkt.target_scene = SCENE_TYPE::CUSTOMS;

				auto session = GET_SERVER_SESSION;
				assert(session);

				auto sendBuffer = MAKE_SEND_BUFFER(changeScenePkt);
				session->DoSend(sendBuffer);
			}
			};
	}

	if (menuBackBtn) {
		menuBackBtn->OnClick = [this]() {
			if (!g_is_single) {
				C_LeaveRoom leavePkt;
				leavePkt.user_id = my_player->GetID();

				auto session = GET_SERVER_SESSION;
				assert(session);

				auto sendBuffer = MAKE_SEND_BUFFER(leavePkt);
				session->DoSend(sendBuffer);
			}
			ui_manager->ToggleUI("LobbyMenuCanvas", false, false);
			CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::TITLE);
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

	// (테스트) 로비씬에 입장하면 딱 한번만 10000원 준다. (나중에 지울 것)
	static bool once = false;
	if (g_is_single && !once) {
		my_player->SetGold(10000);
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
