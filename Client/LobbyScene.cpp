#include "stdafx.h"
#include "LobbyScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"
#include "SceneManager.h"
#include "KeyManager.h"
#include "ImGuiManager.h"

CLobbyScene::CLobbyScene()
	: CScene(SCENE_TYPE::LOBBY)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::Initialize()
{
	// 렌더링할 때 필요한 쉐이더 객체 생성
	if(shaders.empty()) {
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
	}

	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["static"]->GetHeapManager() };
		objects = factory->CreateLobby(staticHeapManager);
	}
}

void CLobbyScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
	}
	else {
		factory->SetComponent(dynamic_pointer_cast<CPlayer>(my_player));
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
	// test 용 삭제X
	{
		/*std::ifstream bin("../Modeling/undead_char.bin", std::ios::binary);
		std::ofstream txt("../Modeling/char.txt");

		char ch;
		while (bin.get(ch)) {
			if (
				ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || (ch >= 'A' && ch <= 'Z') ||
		 (ch >= 'a' && ch <= 'z') || ch == '<' || ch == '>' || ch == '/' )
			{
				txt << ch;
			}
		}*/
	}
}

void CLobbyScene::Update(float elapsedTime)
{
	if (paused)
		return;

	CScene::Update(elapsedTime);
	CPhysicsManager::GetInstance().Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
	}
}

void CLobbyScene::DrawUI()
{
	// 로딩 팝업 (최우선 순위)
	if (loading_type != LoadingType::None) {
		
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

	// 2. 창을 화면 정중앙에 배치
	ImVec2 centerPos = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	// (선택) 메뉴 배경 투명도 설정 (0.8f는 살짝 어두운 반투명, 0.0f는 완전 투명)
	ImGui::SetNextWindowBgAlpha(0.8f);

	// 윈도우 타이틀바, 리사이즈, 이동 기능 모두 제거
	ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

	// 배경 채우기 
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		ImVec2(0, 0), screenSize, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.15f, 0.75f))
	);

	if (ImGui::Begin("Lobby Menu", nullptr, menuFlags))
	{
		// 폰트 스케일링 적용
		ImGui::SetWindowFontScale(scale);
		
		// 버튼 크기도 스케일링 적용
		ImVec2 btnSize = ImVec2(200.0f * scale, 60.0f * scale);
		
		// 여백 살짝 주기
		ImGui::Spacing(); ImGui::Spacing();

		// 방 나가기 버튼 렌더링
		if (ImGui::Button((const char*)u8"방 나가기", btnSize)) {
		
			// CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::TITLE);
		
			SetUIState(LobbyUIState::None); // 버튼 누르면 일단 메뉴 닫기
			paused = false;
		}
		
		ImGui::Spacing(); ImGui::Spacing();

		// 6. 스케일 원상 복구 (필수)
		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();
}

void CLobbyScene::Enter()
{
	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::LOBBY);
		camera->SetTarget(my_player.get());
	}
}

void CLobbyScene::Exit()
{
	
}