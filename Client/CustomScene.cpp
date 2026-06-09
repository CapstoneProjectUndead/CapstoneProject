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
    if (objects.empty()) {
        auto& factory = CSceneManager::GetInstance().GetFactory();
        objects = factory->CreateLobby();
    }
}

void CCustomScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    // 플레이어 생성
    if (!my_player) {
        auto& factory = CSceneManager::GetInstance().GetFactory();
        my_player = factory->CreateMyPlayer();
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
        light->AddPointLight(XMFLOAT3(2.49913216, 1.35, 1.45683444), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 2.0f);			// Lamp on table
        light->AddPointLight(XMFLOAT3(1.83547652, 2.55953884, -2.89159632), XMFLOAT3(1.0f, 0.8f, 0.6f), 1.0f, 2.0f);	// Lamp on StoneBed
        light->AddPointLight(XMFLOAT3(-2.44799995, 0.48300001, -0.216999993), XMFLOAT3(1.0f, 0.8f, 0.6f), 0.5f, 1.5f);	// firewood
        light->AddPointLight(XMFLOAT3(0, 0, 1), XMFLOAT3(0.5f, 0.5f, 0.5f), 1.0f, 2.0f);		// custom
    }
}

void CCustomScene::Enter()
{
    CScene::Enter();

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
    float scale = G_RATIO_Y;

    ImVec2 winSize = ImVec2(300.0f * scale, 280.0f * scale); // 적정 크기 유지
    float margin = 30.0f * scale;

    ImGui::SetNextWindowPos(ImVec2(screenSize.x - winSize.x - margin, screenSize.y - winSize.y - margin));
    ImGui::SetNextWindowSize(winSize);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("CustomUI", nullptr, winFlags))
    {
        ImGui::SetWindowFontScale(scale);

        auto DrawSimpleSelector = [&](const char* partName, int& currentIdx, int maxCount, const char* names[], std::function<void(int)> onChange) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), partName);
            ImVec2 smallBtnSize = ImVec2(30.0f * scale, 30.0f * scale);
            float yOffset = 5.0f * scale;

            if (ImGui::Button((std::string("<##") + partName).c_str(), smallBtnSize)) {
                currentIdx = (currentIdx - 1 + maxCount) % maxCount;
                if (onChange) onChange(currentIdx);
            }
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
            if (names) ImGui::Text(" %s ", names[currentIdx]);
            else ImGui::Text(" Style %d ", currentIdx + 1);
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - yOffset);

            if (ImGui::Button((std::string(">##") + partName).c_str(), smallBtnSize)) {
                currentIdx = (currentIdx + 1) % maxCount;
                if (onChange) onChange(currentIdx);
            }
            ImGui::Spacing();
            };

        // 모델 세트를 깔끔하게 6종으로 확장 정의
        static const char* modelNames[] = { "Dog 1", "Cat 1", "Bunny 1", "Dog 2", "Cat 2", "Bunny 2" };

        // [람다 헬퍼] 값이 바뀔 때마다 플레이어 인덱스를 수정하고 팩토리 마스터 교체 함수를 작동시킴
        auto& factory = CSceneManager::GetInstance().GetFactory();
        auto OnCustomChanged = [&]() {
            if (my_player && factory) { // m_factory는 Scene이 보유한 팩토리 포인터라고 가정
                factory->UpdatePlayerTextures(my_player);
            }
            };

        // 1. 모델 세트 선택 (최대 카운트 6)
        DrawSimpleSelector("MODEL SET", body_idx, 6, modelNames, [&](int idx) {
            if (my_player) my_player->ChangeModelSet(idx);
            OnCustomChanged();
            });

        // 2. 눈 스타일 선택 (eyes_5=기절, eyes_9=빙의 전용 → 커스터마이징 제외, 8종만 노출)
        static const int eyesMap[] = { 0, 1, 2, 3, 5, 6, 7, 9 };
        DrawSimpleSelector("EYES STYLE", eyes_idx, 8, nullptr, [&](int idx) {
            if (my_player) my_player->ChangeEyes(eyesMap[idx]);
            OnCustomChanged();
            });

        // 3. 입 스타일 선택 (1 ~ 10종)
        DrawSimpleSelector("MOUTH STYLE", mouth_idx, 10, nullptr, [&](int idx) {
            if (my_player) my_player->ChangeMouth(idx);
            OnCustomChanged();
            });

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button((const char*)u8"SELECT DONE", ImVec2(150 * scale, 40 * scale))) {
            if (g_is_single) {
                ShowResultPopup(true, "설정 완료!");
            }
            else {
                auto session = my_player->GetSession();
                if (session) {
                    C_CustomSelect selectPkt;
                    selectPkt.player_id = my_player->GetID();
                    selectPkt.body_type = body_idx; // 0 ~ 5의 통합 값이 패킷으로 전송됨
                    selectPkt.eye_type = my_player->GetEyesIndex(); // UI 인덱스가 아닌 실제 텍스처 인덱스 전송
                    selectPkt.mouth_type = mouth_idx;

                    auto sendBuffer = MAKE_SEND_BUFFER(selectPkt);
                    session->DoSend(sendBuffer);
                }
                StartLoading(LoadingType::SelectResult);
            }
        }
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
                if (g_is_single) {
                    CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
                }
                else {
                    auto session = my_player->GetSession();
                    if (session) {
                        C_SceneChange changeScenePkt;
                        changeScenePkt.player_id = my_player->GetID();
                        changeScenePkt.current_scene = scene_type;
                        changeScenePkt.target_scene = SCENE_TYPE::LOBBY;

                        auto sendBuffer = MAKE_SEND_BUFFER(changeScenePkt);
                        session->DoSend(sendBuffer);
                    }
                }

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