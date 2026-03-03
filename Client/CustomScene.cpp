#include "stdafx.h"
#include "CustomScene.h"
#include "Shader.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "ImGuiManager.h"
#include "ObjectFactory.h"
#include "Camera.h"
#include "ServerPacketHandler.h"

void CCustomScene::Initialize()
{
	// 렌더링할 때 필요한 쉐이더 객체 생성
    if (shaders.empty()) {
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

void CCustomScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    // 플레이어 생성
    if (!my_player) {
        CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
        my_player = factory->CreateMyPlayer(skinningHeapManager);
    }
    my_player->SetPitch(-10);   // 얼굴이 잘보이도록 수치 조정

    if (!camera) {
        camera = std::make_shared<CCamera>();
        camera->SetTarget(my_player.get());
        camera->Initialize(device, commandList);
        camera->SetCameraOffset(XMFLOAT3{ 0.0f, 1.0f, 1.0f });
        camera->SetMode(CCamera::EMode::FIXED);
    }

    // light 생성
    if (!light) {
        light = std::make_unique<CLightManager>();
        light->Initialize(device, commandList);
    }
}

void CCustomScene::Update(float elapsedTime)
{
    CScene::Update(elapsedTime);
}

void CCustomScene::Enter()
{
    BuildObjects(GET_DEVICE, GET_CMD_LIST);

    if (my_player) {
        my_player->SetCurrentSceneType(SCENE_TYPE::CUSTOMS);
        my_player->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
        camera->SetTarget(my_player.get());
        camera->SetCameraOffset(XMFLOAT3{ 0.0f, 1.0f, 1.0f });
        camera->SetMode(CCamera::EMode::FIXED);
    }
}

void CCustomScene::Exit()
{
    CScene::Exit();

    my_player = nullptr;
}

bool CCustomScene::IsUIInputEnabled()
{
    bool state = true;

    CScene* scene = CSceneManager::GetInstance().GetActiveScene();
    assert(scene);

    if (scene->GetSceneType() == SCENE_TYPE::CUSTOMS)
        state = false;

    return state;
}


void CCustomScene::DrawUI()
{
	CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
	if (currentScene->GetSceneType() != SCENE_TYPE::CUSTOMS)
		return;

    // 로딩 팝업 (최우선 순위)
    if (loading_type != LoadingType::None) {
        DrawLoadingPopUp();
    }

    // 결과 팝업
    if (pop_up_result.is_visible) {
        DrawLoadingPopUpResult();
    }

    DrawCustomizingWindow();
}

void CCustomScene::DrawCustomizingWindow()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // 화면 크기에 맞게 스케일링
    float scale = G_RATIO_Y;

    // 창 크기를 조금 더 줄였습니다.
    ImVec2 winSize = ImVec2(300.0f * scale, 250.0f * scale);
    float margin = 30.0f * scale;

    // 오른쪽 하단 여백 조정
    ImGui::SetNextWindowPos(ImVec2(screenSize.x - winSize.x - margin, screenSize.y - winSize.y - margin));
    ImGui::SetNextWindowSize(winSize);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("CustomUI", nullptr, winFlags))
    {
        // 글씨 크기도 스케일링
        ImGui::SetWindowFontScale(scale);

        auto DrawSimpleSelector = [&](const char* partName, int& currentIdx, int maxCount, const char* names[], std::function<void(int)> onChange) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), partName);

            // 람다 내부의 작은 좌우 화살표 버튼 크기와 높이 오프셋도 스케일링
            ImVec2 smallBtnSize = ImVec2(30.0f * scale, 30.0f * scale);
            float yOffset = 5.0f * scale;

            // 왼쪽 버튼
            if (ImGui::Button((std::string("<##") + partName).c_str(), smallBtnSize)) {
                currentIdx = (currentIdx - 1 + maxCount) % maxCount;
                if (onChange) onChange(currentIdx); // 변경 시 콜백 호출
            }
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
            ImGui::Text(" %s ", names[currentIdx]);
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - yOffset);
            // 오른쪽 버튼
            if (ImGui::Button((std::string(">##") + partName).c_str(), smallBtnSize)) {
                currentIdx = (currentIdx + 1) % maxCount;
                if (onChange) onChange(currentIdx); // 변경 시 콜백 호출
            }
            ImGui::Spacing();
            };

        static const char* bodyNames[] = { "Dog", "Cat", "Buddy" };
        static const char* eyeNames[] = { "Pretty", "Line", "Side"};
        static const char* mouthNames[] = { "Ganadi","Nya", "Toto"};

        DrawSimpleSelector("BODY", body_idx, 3, bodyNames, [&](int idx) {
            if (my_player) my_player->ChangeModelSet(idx);
            });

        DrawSimpleSelector("EYES", eyes_idx, 3, eyeNames, [&](int idx) {
            if (my_player) my_player->ChangeEyes(idx);
            });

        DrawSimpleSelector("MOUTH", mouth_idx, 3, mouthNames, [&](int idx) {
            if (my_player) my_player->ChangeMouth(idx);
            });

        ImGui::Separator(); // 얇은 구분선 하나 추가
        ImGui::Spacing();

        // 완료 버튼도 적당한 크기로 수정
        if (ImGui::Button((const char*)u8"SELECT DONE", ImVec2(150 * scale, 40 * scale))) {

            if (my_player->GetIsSingle()) {
                CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
            }
            else {
                auto session = my_player->GetSession();
                if (session) {
                    C_CustomSelect selectPkt;
                    selectPkt.player_id = my_player->GetID();
                    selectPkt.body_type = body_idx;
                    selectPkt.eye_type = eyes_idx;
                    selectPkt.mouth_type = mouth_idx;

                    auto sendBuffer = MAKE_SEND_BUFFER(selectPkt);
                    session->DoSend(sendBuffer);
                }

                StartLoading(LoadingType::SelectResult);
            }
        }

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);
    }

    ImGui::End();
}

void CCustomScene::DrawLoadingPopUp()
{
    if (!ImGui::IsPopupOpen("LoadingPopup")) {
        ImGui::OpenPopup("LoadingPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("LoadingPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {

        float scale = G_RATIO_Y;

        ImGui::SetWindowFontScale(scale);

        CImGuiManager::LoadingIndicatorCircle("spinner", 20.0f, ImVec4(0.2f, 0.5f, 1.0f, 1.0f), ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 10, 5.0f);
        ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();

        const char* txt = (const char*)u8"로딩 중...";
        switch (loading_type) {
        case LoadingType::SelectResult:      txt = (const char*)u8"로딩 중..."; break;
        }
        ImGui::Text("%s", txt);

        ImGui::Spacing();
        if (ImGui::Button("Cancel")) {
            StopLoading();
            ImGui::CloseCurrentPopup();
        }

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);

        ImGui::EndPopup();
    }
}

void CCustomScene::ShowResultPopup(bool is_success, const std::string& msg)
{
    pop_up_result.is_visible = true;
    pop_up_result.is_success = is_success;
    pop_up_result.message = msg;
}

void CCustomScene::CloseResultPopup()
{
    pop_up_result.is_visible = false;
    pop_up_result.is_success = false;
}

void CCustomScene::DrawLoadingPopUpResult()
{
    if (!ImGui::IsPopupOpen("ResultPopup")) {
        ImGui::OpenPopup("ResultPopup");
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("ResultPopup", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {

        float scale = G_RATIO_Y;

        ImGui::SetWindowFontScale(scale);

        ImGui::Text("%s", CP949ToUTF8(pop_up_result.message).c_str());
        ImGui::Spacing();

        if (ImGui::Button((const char*)u8"확인")) {

            pop_up_result.is_visible = false; // 팝업 닫기
            ImGui::CloseCurrentPopup();

            if (pop_up_result.is_success) {    
                CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
                CloseResultPopup();
            }
        }

        // 폰트 스케일 원상 복구
        ImGui::SetWindowFontScale(1.0f);

        ImGui::EndPopup();
    }
}

// 서버 패킷 관련 처리 함수들
void CCustomScene::Handle_S_Custom_Select(std::shared_ptr<Session> session, S_CustomSelect& pkt)
{
    ShowResultPopup(true, "설정 완료!");

    CImGuiManager::GetInstance().ReserveResetFocus();

    StopLoading();
}