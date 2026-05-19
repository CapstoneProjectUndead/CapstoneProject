#include "stdafx.h"
#include "LobbyScene.h"
#include "CustomScene.h"
#include "SceneManager.h"
#include "KeyManager.h"
#include "ImGuiManager.h"

#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "ObjectFactory.h"
#include "UIComponent.h"
#include "DataManager.h"

#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "GameFramework.h"

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
	// 버튼 ui
	auto YNCanvas = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/YNButton.json");
	YNCanvas->SetEnable(false);
	ui_manager->AddCanvas(YNCanvas);
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
		auto menuUI = ui_manager->GetUI<CUICanvas>("LobbyMenuCanvas");
		if (menuUI) {
			ui_manager->ToggleUI("LobbyMenuCanvas", !menuUI->is_enable, menuUI->is_enable);
		}
	}

	// C 키로 사신 상호작용
	if (KEY_TAP(KEY::C)) {
		InteractWithReaper();
	}

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

void CLobbyScene::SetButtonEvents()
{
	auto reaperText = ui_manager->GetUI<CUIText>("ReaperText");
	auto yesBtn = ui_manager->GetUI<CUIButton>("YesButton");
	auto noBtn = ui_manager->GetUI<CUIButton>("NoButton");
	auto menuToCustomBtn = ui_manager->GetUI<CUIButton>("ToCustom");
	auto menuBackBtn = ui_manager->GetUI<CUIButton>("Back");

	// 대사가 끝났을 때 버튼을 보여주는 함수 등록
	if (reaperText) {
		reaperText->onFinished = [this]() {
			ui_manager->ToggleUI("YNCanvas", true);
			};
	}

	// 버튼 콜백 함수 등록
	if (yesBtn) {
		yesBtn->OnClick = [this]() {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			ui_manager->ToggleUI("YNCanvas", false, true);
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
		noBtn->OnClick = [this]() {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			ui_manager->ToggleUI("YNCanvas", false, true);
			};
	}

	if (menuToCustomBtn) {
		menuToCustomBtn->OnClick = [this]() {
			ui_manager->ToggleUI("LobbyMenuCanvas", false, false);

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
			}
			else {
				// 멀티는 서버 응답(S_SceneChange) 받을 때 전환됨
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
		my_player->SetIsReady(false);
		my_player->SetReturned(false);
		camera->SetTarget(my_player.get());
	}

	auto readyUI = ui_manager->GetUI<CUIImage>("Ready" + std::to_string(1));
	if (readyUI)
		readyUI->SetColor(XMFLOAT4{ 1, 0, 0, 1 });
}

void CLobbyScene::Exit()
{
	CScene::Exit();

	my_player = nullptr;
}

bool CLobbyScene::IsUIInputEnabled()
{
	bool state = true;

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene);

	if (scene->GetSceneType() == SCENE_TYPE::LOBBY)
		state = false;
		
	return state;
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
