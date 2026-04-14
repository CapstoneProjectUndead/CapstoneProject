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

	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager{ shaders["inst"]->GetHeapManager() };
	if (objects.empty()) {
		objects = factory->CreateLobby(heapManager);
	}

	// UI 생성
	auto mainCanvas = ui_manager->CreateCanvas();
	auto repeaper = factory->CreateReaper(heapManager);

	objects.push_back(repeaper);

	// LoadData
	ui_manager->GetDataManager()->LoadScripts("../Modeling/UI/Reaper.json");
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


	SetupDialogueEvents();
}

void CLobbyScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
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
	if (paused)
		return;

	CScene::Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
		UpdatePlayerReadyUI();
	}

	// 테스트 (나중에 지울 것)
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
	text->SetText(ui_manager->GetDataManager()->GetDialogue("Reaper", "Ask_Exit"));
}

void CLobbyScene::SetupDialogueEvents()
{
	auto reaperText = ui_manager->GetUI<CUIText>("ReaperText");
	auto yesBtn = ui_manager->GetUI<CUIButton>("YesButton");
	auto noBtn = ui_manager->GetUI<CUIButton>("NoButton");

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
			if (!g_is_single) {
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
			else {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::GAME);
			}
		};
	}

	if (noBtn) {
		noBtn->OnClick = [this]() {
			ui_manager->ToggleUI("ReaperSpeechCanvas", false, true);
			ui_manager->ToggleUI("YNCanvas", false, true);
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
	}
}

void CLobbyScene::Exit()
{
	CScene::Exit();

	my_player = nullptr;
	paused = false; 
	StopLoading();
}

void CLobbyScene::DrawUI()
{
	// 로딩 팝업 (최우선 순위)
	if (loading_type != LoadingType::None) {
		DrawLoadingPopUp();
	}

	// 결과 팝업
	if (pop_up_result.is_visible) {
		
	}

	// 상태에 따른 UI 분기
	switch (ui_state)
	{
	case LobbyUIState::Menu:
		DrawMenu();
		break;
	case LobbyUIState::None:
		if (KEY_TAP(KEY::ESC)) {
			SetUIState(LobbyUIState::Menu);
			paused = true;
		}
		break;
	}
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

void CLobbyScene::DrawMenu()
{
	if (KEY_TAP(KEY::ESC)) {
		SetUIState(LobbyUIState::None);
		paused = false;
	}

	ImVec2 screenSize = ImGui::GetIO().DisplaySize;

	float scale = G_RATIO_Y;

	// 창을 화면 정중앙에 배치
	ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	// 윈도우 타이틀바, 리사이즈, 이동 기능 모두 제거
	ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove 
		| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;

	// 배경 채우기 
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		ImVec2(0, 0), screenSize, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.75f))
	);

	if (ImGui::Begin("Lobby Menu", nullptr, menuFlags)) {
		// 폰트 스케일링 적용
		ImGui::SetWindowFontScale(scale);
		
		// 버튼 크기도 스케일링 적용
		ImVec2 btnSize = ImVec2(200.0f * scale, 60.0f * scale);
		
		// 여백 살짝 주기
		ImGui::Spacing(); ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));        // 평소 색상
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // 마우스 올렸을 때
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));  // 클릭했을 때

		// 커스텀 버튼 렌더링 (방 나가기 버튼 바로 아래에 생성됨)
		if (ImGui::Button((const char*)u8"커스텀", btnSize)) {

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
				SetUIState(LobbyUIState::None);
				paused = false;
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

				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::CUSTOMS);
				SetUIState(LobbyUIState::None);
				paused = false;
			}
		}

		ImGui::Spacing(); 
		ImGui::Spacing();

		// 방 나가기 버튼 렌더링
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
	// =============
	// [Modal PopUp]
	// =============

	ImVec2 screenSize = ImGui::GetIO().DisplaySize;
	float scale = G_RATIO_Y;

	ImVec2 popupCenter = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f);
	ImGui::SetNextWindowPos(popupCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	// BeginPopupModal은 OpenPopup이 호출되었을 때만 true를 반환.
	if (ImGui::BeginPopupModal((const char*)u8"LeaveConfirmPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {

		ImGui::SetWindowFontScale(scale);

		ImGui::Text((const char*)u8"정말로 나가시겠습니까?");
		ImGui::Separator();
		ImGui::Spacing();

		// 팝업 안의 버튼 크기 세팅
		ImVec2 popupBtnSize = ImVec2(100.0f * scale, 40.0f * scale);

		// [확인 버튼]
		if (ImGui::Button((const char*)u8"확인", popupBtnSize)) {
			
			if (!g_is_single) {
				C_LeaveRoom leavePkt;
				leavePkt.user_id = my_player->GetID();

				auto session = GET_SERVER_SESSION;
				assert(session);

				auto sendBuffer = MAKE_SEND_BUFFER(leavePkt);
				session->DoSend(sendBuffer);
			}

			CCustomScene* customScene = (CCustomScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get();
			customScene->body_idx = 0;
			customScene->eyes_idx = 0;
			customScene->mouth_idx = 0;

			CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::TITLE);

			ImGui::CloseCurrentPopup(); // 팝업 닫기
			SetUIState(LobbyUIState::None); // 전체 ESC 메뉴 닫기
			paused = false;

			g_is_single = true; // 다시 싱글 모드 
		}

		ImGui::SameLine(); 

		// [취소 버튼]
		if (ImGui::Button((const char*)u8"취소", popupBtnSize)) {
			ImGui::CloseCurrentPopup(); // 팝업만 닫기 (ESC 메뉴는 그대로 둠)
		}

		ImGui::SetWindowFontScale(1.0f);

		ImGui::EndPopup();
	}
}

void CLobbyScene::DrawLoadingPopUp()
{
	if (!ImGui::IsPopupOpen("LoadingPopup")) {
		ImGui::OpenPopup("LoadingPopup");
	}

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {

		float scale = G_RATIO_Y;

		CImGuiManager::LoadingIndicatorCircle("spinner", 20.0f * scale, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);
		ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

		ImGui::SetWindowFontScale(scale);

		const char* txt = "로딩 중...";
		switch (loading_type) {
		case LoadingType::MapLoading: txt = (const char*)u8"맵 로딩 중..."; break;
		case LoadingType::GenerateMap: txt = (const char*)u8"맵 생성 중..."; break;
		}
		ImGui::Text("%s", txt);

		// 폰트 스케일 원상 복구
		ImGui::SetWindowFontScale(1.0f);

		ImGui::EndPopup();
	}
}

// 서버 패킷 처리 관련 함수들
void CLobbyScene::Handle_S_MapStart(std::shared_ptr<Session> session, const S_MapStart& pkt)
{
	StartLoading(LoadingType::GenerateMap);
	paused = true;
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

	auto readyUI = ui_manager->FindUI<CUIImage>("Ready" + std::to_string(slot));
	if (readyUI)
		readyUI->SetColor(XMFLOAT4{ 0, 0, 1, 1 });
}
