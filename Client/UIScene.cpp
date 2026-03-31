#include "stdafx.h"
#include "UIScene.h"
#include "UIComponent.h"
#include "ImGuiManager.h"
#include "Renderers.h"
#include "Shader.h"
#include "GameFramework.h"
#include "Camera.h"
#include "ObjectFactory.h"
#include "MyPlayer.h"
#include "SceneManager.h"
#include "DataManager.h"

CUIScene::CUIScene()
	:CScene(SCENE_TYPE::UI)
{
}

void CUIScene::Initialize()
{
	CScene::Initialize();

    auto shaders = CSceneManager::GetInstance().GetShaders();
    {
        // UI
        std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
        shader->CreateShader(GET_DEVICE);
        shaders.emplace("ui", std::move(shader));
    }
}

void CUIScene::Update(float dt)
{
    ui_manager->Update(dt);

	// 마우스 드래그 이동 로직 (선택된 UI가 있을 때)
	HandleUIDragging();
}

void CUIScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!camera) {
        camera = std::make_shared<CCamera>();
        camera->Initialize(device, commandList);
    }
}

void CUIScene::Render(ID3D12GraphicsCommandList* commandList)
{
    auto& shaders = CSceneManager::GetInstance().GetShaders();
    auto& renderers = CSceneManager::GetInstance().GetRanderers();

    ui_manager->Collect(renderers);

    for (const auto& [name, pShader] : shaders) {
        pShader->RenderBegin(commandList);

        camera->UpdateShaderVariables(commandList, true);

        auto it = renderers.find(name);
        if (it != renderers.end()) {
            it->second->Render(commandList);
            if (name == "ui") {
                renderers["text"]->Render(commandList);
            }
        }

        pShader->RenderEnd(commandList);
    }
}

void CUIScene::DrawUI()
{
	RenderHierarchyWindow();
	RenderInspectorWindow();
	RenderToolbar();
}

bool CUIScene::IsUIInputEnabled()
{
	return false;
}

void CUIScene::RenderHierarchyWindow()
{
    ImGui::Begin("UI Hierarchy");

    // 생성 툴바
    if (ImGui::Button("Add Canvas")) {
        // 새 캔버스를 생성하고 바로 편집 대상으로 설정
        selected_UI = ui_manager->CreateCanvas();
    }

    ImGui::SameLine();
    // 자식 추가 버튼들 (선택된 UI가 있을 때만 활성화)
    bool hasSelection = (selected_UI != nullptr);
    if (!hasSelection) ImGui::BeginDisabled();

    if (ImGui::Button("Add Image")) {
        selected_UI->AddChild(std::make_shared<CUIImage>());
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Button")) {
        selected_UI->AddChild(std::make_shared<CUIButton>());
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Text")) {
        selected_UI->AddChild(std::make_shared<CUIText>());
    }

    if (!hasSelection) ImGui::EndDisabled();

    ImGui::Separator();

    // 모든 캔버스 순회 출력
    auto& allCanvases = ui_manager->GetCanvases();
    for (auto& canvas : allCanvases) {
        DrawHierarchyNode(canvas);
    }

    ImGui::End();
}


void CUIScene::DrawHierarchyNode(std::shared_ptr<CUIComponent> node)
{
    ImGuiTreeNodeFlags flags = (selected_UI == node) ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;

    // 이름 + 주소를 같이 출력하면 중복 이름 구분이 편함
    std::string label = node->GetName() + "##" + std::to_string((uintptr_t)node.get());

    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

    // 클릭 시 선택
    if (ImGui::IsItemClicked()) {
        selected_UI = node;
    }

    if (opened) {
        for (auto& c : node->GetChildren()) {
            DrawHierarchyNode(c);
        }
        ImGui::TreePop();
    }
}

void CUIScene::RenderInspectorWindow()
{
    // 창이 항상 보이도록 설정
    ImGui::Begin("Inspector");

    if (selected_UI) {
        // 이름 변경 및 삭제
        char nameBuf[128];
        strcpy_s(nameBuf, selected_UI->GetName().c_str());
        if (ImGui::InputText("Name", nameBuf, 128)) {
            selected_UI->SetName(nameBuf);
        }

        // 삭제 버튼: UI_Element(캔버스)는 삭제 X
        if (selected_UI != editor_canvas) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Delete Element", ImVec2(-1, 0))) {
                DeleteSelectedUI();
                ImGui::PopStyleColor();
                ImGui::End();
                return;
            }
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // Transform
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            XMFLOAT2 pos = selected_UI->GetRelativePos();
            XMFLOAT2 size = selected_UI->GetSize();
            XMFLOAT2 pivot = selected_UI->GetPivot();
            XMFLOAT2 anchor = selected_UI->GetAnchor();

            // ImGui UI 출력 및 수정 여부 체크
            if (ImGui::DragFloat2("Pos (Rel)", (float*)&pos, 1.0f)) {
                selected_UI->SetRelativePos(pos); // 값이 변하면 Set 호출 -> Invalidate 작동
            }
            if (ImGui::DragFloat2("Size", (float*)&size, 1.0f)) {
                if (size.x < 0) size.x = 0; if (size.y < 0) size.y = 0;
                selected_UI->SetSize(size);
            }
            if (ImGui::DragFloat2("Pivot", (float*)&pivot, 0.01f, 0.0f, 1.0f)) {
                selected_UI->SetPivot(pivot);
            }
            if (ImGui::DragFloat2("Anchor", (float*)&anchor, 0.01f, -1.0f, 1.0f)) {
                selected_UI->SetAnchor(anchor);
            }
        }

        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
            XMFLOAT4 col = selected_UI->GetColor();
            if (ImGui::ColorEdit4("Color", (float*)&col)) {
                selected_UI->SetColor(col);
            }
        }
    }

    ImGui::End();
}

void CUIScene::RenderToolbar()
{
    ImGui::Begin("Toolbar");

    // --- Copy / Paste ---
    if (ImGui::Button("Copy (Ctrl+C)")) CopySelectedUI();
    ImGui::SameLine();
    if (ImGui::Button("Paste (Ctrl+V)")) {
        PasteUI();
    }

    ImGui::Separator();

    ImGui::InputText("File Path", save_path, 256);

    auto dataManager = ui_manager->GetDataManager();

    // 현재 선택된 UI가 속한 최상위 캔버스를 찾음
    auto currentTargetCanvas = std::dynamic_pointer_cast<CUICanvas>(selected_UI);

    if (ImGui::Button("Save Selected Canvas")) {
        if (currentTargetCanvas)
            dataManager->SaveToFile(currentTargetCanvas, save_path);
    }

    ImGui::SameLine();

    if (ImGui::Button("Load As New Canvas")) {
        auto newCanvas = dataManager->LoadFromFile(save_path);
        if (newCanvas) {
            ui_manager->AddCanvas(newCanvas); // 리스트에 추가
            selected_UI = newCanvas;          // 로드 후 바로 선택
        }
    }
    ImGui::End();
    // 단축키 처리
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        if (ImGui::IsKeyPressed(ImGuiKey_C)) CopySelectedUI();
        if (ImGui::IsKeyPressed(ImGuiKey_V)) { PasteUI(); }
    }
}

void CUIScene::HandleUIDragging()
{
    // 1. 선택된 UI가 있고
    // 2. 마우스를 왼쪽 클릭으로 드래그 중이며
    // 3. ImGui가 마우스를 점유하지 않았을 때 (게임 뷰포트 클릭 시)
    if (selected_UI && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;

        // Getter로 값을 가져와서 계산 후 Setter로 주입
        XMFLOAT2 pos = selected_UI->GetRelativePos();
        pos.x += delta.x;
        pos.y += delta.y;

        selected_UI->SetRelativePos(pos); // 여기서 Invalidate가 일어남
    }
}

// 부모 UI를 재귀적으로 찾음
std::shared_ptr<CUIComponent> CUIScene::FindParent(std::shared_ptr<CUIComponent> root, std::shared_ptr<CUIComponent> target)
{
    if (!root) return nullptr;

    for (auto& c : root->GetChildren()) {
        if (c == target) return root;
        auto found = FindParent(c, target);
        if (found) return found;
    }
    return nullptr;
}

void CUIScene::DeleteSelectedUI()
{
    if (!selected_UI) return;

    // 일반적인 자식 UI인 경우 (부모가 있음)
    bool deleted = false;
    auto& allCanvases = ui_manager->GetCanvases();

    for (auto& canvas : allCanvases) {
        if (canvas == selected_UI) {
            // 캔버스 자체를 삭제하는 경우
            allCanvases.erase(std::remove(allCanvases.begin(), allCanvases.end(), canvas), allCanvases.end());
            deleted = true;
            break;
        }

        auto parent = FindParent(canvas, selected_UI);
        if (parent) {
            auto& children = parent->GetChildren();
            children.erase(std::remove(children.begin(), children.end(), selected_UI), children.end());
            deleted = true;
            break;
        }
    }

    selected_UI = nullptr;
}

void CUIScene::CopySelectedUI()
{
    if (!selected_UI) return;
    clipboard = selected_UI->Serialize();
}

void CUIScene::PasteUI()
{
    if (clipboard.empty() || !selected_UI) return;

    // LoadRecursive를 활용해 클립보드 데이터로 새 UI 생성
    // 선택된 UI의 자식으로 붙여넣기 시전
    auto dataManager = ui_manager->GetDataManager();
    dataManager->LoadRecursive(selected_UI, clipboard);
}