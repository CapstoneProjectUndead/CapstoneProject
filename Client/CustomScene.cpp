#include "stdafx.h"
#include "CustomScene.h"
#include "Shader.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "ImGuiManager.h"
#include "ObjectFactory.h"
#include "Camera.h"

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
}

void CCustomScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    // 플레이어 생성
    if (!my_player) {
        CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
        my_player = std::make_shared<CMyPlayer>();
        factory->CreateUndeadCharacter(my_player, skinningHeapManager);
    }
    my_player->SetPitch(-10);   // 얼굴이 잘보이도록 수치 조정

    if(objects.empty()) {
        CDescriptorHeapManager* staticHeapManager{ shaders["static"]->GetHeapManager() };
        objects = factory->CreateLobby(staticHeapManager);
    }

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

void CCustomScene::Enter()
{
    BuildObjects(GET_DEVICE, GET_CMD_LIST);

    if (my_player) {
        my_player->SetCurrentSceneType(SCENE_TYPE::CUSTOMS);
        camera->SetTarget(my_player.get());
    }
}

void CCustomScene::Exit()
{

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

    DrawCustomizingWindow();
}

void CCustomScene::DrawCustomizingWindow()
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    // 창 크기를 조금 더 줄였습니다.
    ImVec2 winSize = ImVec2(300, 250);

    // 오른쪽 하단 여백 조정
    ImGui::SetNextWindowPos(ImVec2(screenSize.x - winSize.x - 30, screenSize.y - winSize.y - 30));
    ImGui::SetNextWindowSize(winSize);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("CustomUI", nullptr, winFlags))
    {
        auto DrawSimpleSelector = [&](const char* partName, int& currentIdx, int maxCount, const char* names[], std::function<void(int)> onChange) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), partName);

            // 왼쪽 버튼
            if (ImGui::Button((std::string("<##") + partName).c_str(), ImVec2(30, 30))) {
                currentIdx = (currentIdx - 1 + maxCount) % maxCount;
                if (onChange) onChange(currentIdx); // 변경 시 콜백 호출
            }
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            ImGui::Text(" %s ", names[currentIdx]);
            ImGui::SameLine();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
            // 오른쪽 버튼
            if (ImGui::Button((std::string(">##") + partName).c_str(), ImVec2(30, 30))) {
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
        if (ImGui::Button((const char*)u8"SELECT DONE", ImVec2(150, 40))) {
            CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
        }
    }

    ImGui::End();
}