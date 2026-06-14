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

	// 사신(저승사자) NPC 오브젝트 생성
	auto repeaper = factory->CreateReaper();

	objects.push_back(repeaper);

	// 사신 대화창은 ImGui(DrawReaperDialog)로 그림 — Reaper.json/ReaperDialogue.json 로드하지 않음
	// 준비 상태 UI도 ImGui(DrawReadyPanel) — PlayerReady.json 로드하지 않음
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
	}


	bool isInvOpen = (my_player && my_player->GetInventory() && my_player->GetInventory()->IsOpen());
	if (KEY_TAP(KEY::ESC)) {
		// 1순위: 상점이 열려 있으면 상점 닫기
		if (CShop::GetInstance().IsOpen()) {
			CShop::GetInstance().Close();
		}
		// 2순위: 사신 대화창이 열려 있으면 닫기
		else if (reaper_dialog_open) {
			reaper_dialog_open = false;
			CKeyManager::GetInstance().SetMouseMode(true);   // 게임 커서 모드 복귀
		}
		// 3순위: 인벤토리가 닫혀 있을 때만 ESC 메뉴를 켬 (일시정지)
		else if (!isInvOpen) {
			ui_state = LobbyUIState::Menu;
			paused   = true;
			CKeyManager::GetInstance().SetMouseMode(false);  // UI 커서 모드
		}
	}

	// C 키로 위치 기반 상호작용 (인벤토리/대화창이 닫혀 있을 때만 작동 가능)
	if (KEY_TAP(KEY::C) && my_player && !CShop::GetInstance().IsOpen() && !isInvOpen && !reaper_dialog_open) {
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
}

void CLobbyScene::InteractWithReaper()
{
	// 사신 "Ask_Exit" 대사 3줄 중 랜덤 1줄 (구 Reaper.json GetDialogue의 rand 동작 유지)
	static const char* kAskExit[] = {
		(const char*)u8"산 자들의 땅으로 발을 들이겠나?",
		(const char*)u8"준비는 끝났겠지.",
		(const char*)u8"떠날 것인가?",
	};
	reaper_dialog_text = kAskExit[rand() % 3];

	reaper_dialog_open = true;
	CKeyManager::GetInstance().SetMouseMode(false);   // UI 커서 모드(버튼 클릭용)
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

	// 사신 대화창이 열려 있으면 안내를 숨겨 겹침 방지
	if (reaper_dialog_open)
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

void CLobbyScene::DrawReaperDialog()
{
	ImVec2 screen = ImGui::GetIO().DisplaySize;

	// 하단 1/3을 꽉 채우는 가로 바
	float  barH   = screen.y / 3.0f;
	ImGui::SetNextWindowPos(ImVec2(0.0f, screen.y - barH), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(screen.x, barH), ImGuiCond_Always);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.62f, 0.62f, 0.64f, 0.96f));   // 회색 바
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::Begin("ReaperDialog", nullptr, flags)) {
		ImGui::SetWindowFontScale(G_RATIO_Y);

		// 버튼(오른쪽 세로 2개) 영역
		float bw    = 160.0f * G_RATIO_X;
		float bh    = 52.0f  * G_RATIO_Y;
		float vgap  = 20.0f  * G_RATIO_Y;
		float rmarg = 70.0f  * G_RATIO_X;
		float btnX  = screen.x - bw - rmarg;
		float btnY0 = (barH - (2.0f * bh + vgap)) * 0.5f;

		// 사신 대사 (왼쪽, 세로 중앙, 버튼 앞까지 wrap)
		float textX    = 60.0f * G_RATIO_X;
		float textWrap = btnX - 40.0f * G_RATIO_X;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
		ImVec2 textSize = ImGui::CalcTextSize(reaper_dialog_text.c_str(), nullptr, false, textWrap - textX);
		ImGui::SetCursorPos(ImVec2(textX, (barH - textSize.y) * 0.5f));
		ImGui::PushTextWrapPos(textWrap);
		ImGui::TextUnformatted(reaper_dialog_text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();

		// 버튼 색 (파랑 계열)
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.26f, 0.45f, 0.85f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.36f, 0.55f, 0.95f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.20f, 0.35f, 0.70f, 1.0f));

		// [예] → 싱글: 게임 시작 / 멀티: 준비완료(C_Ready) 전송
		ImGui::SetCursorPos(ImVec2(btnX, btnY0));
		if (ImGui::Button((const char*)u8"예", ImVec2(bw, bh))) {
			reaper_dialog_open = false;
			CKeyManager::GetInstance().SetMouseMode(true);

			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::GAME);
			}
			else {
				std::shared_ptr<CMyPlayer> myPlayer = GetMyPlayer();
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
		}

		// [아니오] → 닫기
		ImGui::SetCursorPos(ImVec2(btnX, btnY0 + bh + vgap));
		if (ImGui::Button((const char*)u8"아니오", ImVec2(bw, bh))) {
			reaper_dialog_open = false;
			CKeyManager::GetInstance().SetMouseMode(true);
		}

		ImGui::PopStyleColor(3);
	}
	ImGui::End();

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();
}

void CLobbyScene::DrawReadyPanel()
{
	// 준비 플로우는 멀티 전용
	if (g_is_single || !my_player)
		return;

	// 표시할 참가자 목록 구성: (이름(UTF-8), 준비 여부, 본인 여부)
	struct Row { std::string name; bool ready; bool isSelf; };
	std::vector<Row> rows;

	// 슬롯1: 본인
	rows.push_back({ CP949ToUTF8(my_player->GetName()), my_player->GetIsReady(), true });

	// 슬롯2~4: 타인 (player_slot_ids 순서, ID로 객체 해석)
	// player_slot_ids는 RemoveObject로 정리되지 않아 중복 id가 남을 수 있으므로 표시 단계에서 중복/본인 제외.
	auto& objects  = GetObjects();
	auto& indexMap = GetIDIndex();
	std::unordered_set<uint64> seen;
	seen.insert(my_player->GetID());	// 본인 id는 슬롯1에서 이미 처리

	for (uint64 id : player_slot_ids) {
		if (!seen.insert(id).second)
			continue;	// 이미 표시한 id(중복) → 건너뜀

		auto it = indexMap.find(id);
		if (it == indexMap.end() || it->second >= objects.size())
			continue;	// 이미 나간 플레이어 등 → 건너뜀

		auto other = std::static_pointer_cast<CPlayer>(objects[it->second]);
		if (!other)
			continue;

		rows.push_back({ CP949ToUTF8(other->GetName()), ready_player_ids.count(id) > 0, false });
	}

	// 우상단 배치
	ImVec2 screen = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(screen.x - 10.0f * G_RATIO_X, 20.0f * G_RATIO_Y),
		ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(200.0f * G_RATIO_X, 0.0f), ImGuiCond_Always);

	// 밝은 패널 배경 (행 영역). 헤더 띠는 아래에서 별도 색으로 덮음.
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.90f, 0.91f, 0.94f, 0.94f));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;

	if (ImGui::Begin("ReadyPanel", nullptr, flags)) {
		ImGui::SetWindowFontScale(G_RATIO_Y);

		ImDrawList*       dl    = ImGui::GetWindowDrawList();
		const ImGuiStyle& style = ImGui::GetStyle();

		// ── 헤더 띠: 창 폭 전체를 짙은 남색으로 덮고 흰 글자 (행 배경과 구분) ──
		{
			char header[32];
			sprintf_s(header, (const char*)u8"참가자 (%d/4)", (int)rows.size());

			ImVec2 winPos  = ImGui::GetWindowPos();
			float  winW    = ImGui::GetWindowSize().x;
			ImVec2 textPos = ImGui::GetCursorScreenPos();
			float  lineH   = ImGui::GetTextLineHeight();

			dl->AddRectFilled(
				ImVec2(winPos.x, textPos.y - style.WindowPadding.y),
				ImVec2(winPos.x + winW, textPos.y + lineH + style.WindowPadding.y * 0.5f),
				IM_COL32(40, 52, 74, 255));   // 짙은 남색 띠

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::TextUnformatted(header);
			ImGui::PopStyleColor();
		}

		ImGui::Spacing();

		// ── 참가자 행: 색박스 + 이름(어두운 글자) ──
		const float box = 14.0f * G_RATIO_Y;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));   // 어두운 글자
		for (const auto& r : rows) {
			ImVec2 p   = ImGui::GetCursorScreenPos();
			ImU32  col = r.ready ? IM_COL32(46, 184, 92, 255)    // 준비완료 = 초록
								 : IM_COL32(214, 64, 64, 255);    // 대기중   = 빨강
			dl->AddRectFilled(p, ImVec2(p.x + box, p.y + box), col);

			ImGui::Dummy(ImVec2(box, box));
			ImGui::SameLine();

			std::string label = r.name;
			if (r.isSelf)
				label += (const char*)u8" (나)";
			ImGui::TextUnformatted(label.c_str());
		}
		ImGui::PopStyleColor();
	}
	ImGui::End();

	ImGui::PopStyleColor();
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

	// 준비 상태 초기화 (재진입 시 이전 준비 잔재 제거)
	ready_player_ids.clear();

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

	// 플레이어 HUD (HP/스태미나/가방/조준점)
	my_player->DrawHUD();

	// 준비 상태 패널 (멀티 전용, 우상단)
	DrawReadyPanel();

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

	// 사신 대화창 (입구 C키)
	if (reaper_dialog_open)
		DrawReaperDialog();

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
	// 본인은 my_player->GetIsReady()로 판단하므로 타인만 집합에 기록.
	// (본인 id가 와도 넣어두면 무해하나, DrawReadyPanel은 본인을 GetIsReady로 본다)
	if (my_player && pkt.player_id == my_player->GetID()) {
		my_player->SetIsReady(true);
		return;
	}

	ready_player_ids.insert(pkt.player_id);
}

void CLobbyScene::Handle_S_RefreshStore(std::shared_ptr<Session> session, const S_RefreshStore& pkt)
{
	if (my_player->GetID() != pkt.player_id)
		return;

	CShop::GetInstance().Reset();
}
