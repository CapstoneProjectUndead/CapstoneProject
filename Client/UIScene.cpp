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

CUIScene::CUIScene()
	:CScene(SCENE_TYPE::UI)
{
}

void CUIScene::Initialize()
{
    auto mainCanvas = ui_manager->CreateCanvas();
    editor_canvas = mainCanvas;
    {
        // UI
        std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
        shader->CreateShader(GET_DEVICE);
        shaders.emplace("ui", std::move(shader));
    }
    factory->GetMaterial(shaders["ui"]->GetHeapManager(), "white");	// 인덱스 0에 생성하기 위해 먼저 생성

    auto uiRenderer = std::make_unique<CUIRenderer>();
    uiRenderer->Initialize(GET_DEVICE, 100);
    renderers["ui"] = std::move(uiRenderer);
    // 0번은 white
    CDescriptorHeapManager* heap = shaders["ui"]->GetHeapManager();
    auto textRenderer = std::make_unique<CTextRenderer>();
    textRenderer->Initialize(GET_DEVICE, GET_CMD_QUEUE, heap->GetCPUHandle(1), heap->GetGPUHandle(1));
    renderers["text"] = std::move(textRenderer);
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
    ui_manager->Collect(renderers);

    for (const auto& [name, pShader] : shaders) {
        pShader->RenderBegin(commandList);

        camera->UpdateShaderVariables(commandList, true);

        auto it = renderers.find(name);
        if (it != renderers.end()) {
            it->second->Render(commandList);
            renderers["text"]->Render(commandList);
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

void CUIScene::SaveToFile()
{
    if (!editor_canvas) return;

    // 1. 전체 계층을 json 객체로 변환
    json rootData = editor_canvas->Serialize();

    // 2. 파일 쓰기
    std::ofstream file(save_path);
    if (file.is_open()) {
        file << rootData.dump(4); // 4칸 들여쓰기로 보기 좋게 저장
        file.close();
    }
}

void CUIScene::LoadFromFile()
{
    std::ifstream file(save_path);
    if (!file.is_open()) return;

    json rootData;
    file >> rootData;
    file.close();

    // 기존 데이터 청소 (캔버스의 자식들을 모두 비움)
    editor_canvas->GetChildren().clear();
    selected_UI = nullptr;

    // 캔버스 자체 데이터 복구
    editor_canvas->Deserialize(rootData);

    // 자식들 생성 (재귀 함수 호출)
    if (rootData.contains("Children")) {
        for (const auto& childData : rootData["Children"]) {
            LoadRecursive(editor_canvas, childData);
        }
    }
}

// 역직렬화 헬퍼 함수: 타입에 맞는 객체를 생성하고 자식을 연결함
void CUIScene::LoadRecursive(std::shared_ptr<CUIComponent> parent, const json& data)
{
    std::string type = data["Type"];
    std::shared_ptr<CUIComponent> newUI = nullptr;

    // 1. 타입에 맞는 객체 생성 (ObjectFactory를 사용해도 좋음)
    if (type == "Image") newUI = std::make_shared<CUIImage>();
    else if (type == "Button") newUI = std::make_shared<CUIButton>();
    else newUI = std::make_shared<CUIComponent>();

    // 2. 데이터 세팅
    newUI->Deserialize(data);

    // 3. 계층 구조 연결
    parent->AddChild(newUI);

    // 4. 이 녀석의 자식들도 생성
    if (data.contains("Children")) {
        for (const auto& childData : data["Children"]) {
            LoadRecursive(newUI, childData);
        }
    }
}

bool CUIScene::IsUIInputEnabled()
{
	return false;
}

void CUIScene::RenderHierarchyWindow()
{
    ImGui::Begin("UI Hierarchy");
    if (ImGui::Button("Add Image")) {
        auto img = std::make_shared<CUIImage>();
        editor_canvas->AddChild(img);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Button")) {
        auto btn = std::make_shared<CUIButton>();
        editor_canvas->AddChild(btn);
    }

    ImGui::Separator();
    // 재귀적으로 트리 출력
    DrawHierarchyNode(editor_canvas);
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
            // 위치 조절
            ImGui::DragFloat2("Pos (Rel)", (float*)&selected_UI->GetRelativePos(), 1.0f);

            // 사이즈 조절
            if (ImGui::DragFloat2("Size", (float*)&selected_UI->GetSize(), 1.0f)) {
                // 음수 크기 방지
                auto& s = selected_UI->GetSize();
                if (s.x < 0) s.x = 0;
                if (s.y < 0) s.y = 0;
            }

            ImGui::DragFloat2("Pivot", (float*)&selected_UI->GetPivot(), 0.0f, 1.0f);
            ImGui::DragFloat2("Anchor", (float*)&selected_UI->GetAnchor(), -1.0f, 1.0f);
        }

        // Style
        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit4("Color", (float*)&selected_UI->GetColor());
        }
    }
    else {
        ImGui::Text("Select an item in Hierarchy\nto see properties.");
    }

    ImGui::End();
}

void CUIScene::RenderToolbar()
{
    ImGui::Begin("Toolbar");
    ImGui::InputText("File Path", save_path, 256);
    if (ImGui::Button("Save UI")) SaveToFile();
    if (ImGui::Button("Load UI")) LoadFromFile();
    ImGui::End();
}

void CUIScene::HandleUIDragging()
{
    // 1. 선택된 UI가 있고
    // 2. 마우스를 왼쪽 클릭으로 드래그 중이며
    // 3. ImGui가 마우스를 점유하지 않았을 때 (게임 뷰포트 클릭 시)
    if (selected_UI && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        auto& pos = selected_UI->GetRelativePos();

        pos.x += delta.x;
        pos.y += delta.y;
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
    if (!selected_UI || selected_UI == editor_canvas) return;

    auto parent = FindParent(editor_canvas, selected_UI);
    if (parent) {
        auto& children = parent->GetChildren();
        children.erase(std::remove(children.begin(), children.end(), selected_UI), children.end());
    }

    selected_UI = nullptr; // 삭제 후 선택 해제
}