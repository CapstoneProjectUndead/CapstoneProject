#pragma once
#include "Scene.h"

class CUICanvas;
class CUIComponent;
class IRenderer;
class CShader;

// 실제 게임에서 사용X
class CUIScene : public CScene {
public:
	CUIScene();
    virtual void Initialize() override;

    virtual void Update(float dt) override;
	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;

	virtual void DrawUI() override;
	virtual bool IsUIInputEnabled() override;
private:
    void RenderHierarchyWindow();
    void DrawHierarchyNode(std::shared_ptr<CUIComponent> node);
    void RenderInspectorWindow();
    void RenderToolbar();
    void HandleUIDragging();
    void DeleteSelectedUI();
    std::shared_ptr<CUIComponent> FindParent(std::shared_ptr<CUIComponent> root, std::shared_ptr<CUIComponent> target);

    // UI 내용 저장 및 로드
    void SaveToFile();
    void LoadFromFile();
    void LoadRecursive(std::shared_ptr<CUIComponent> parent, const json& data);
private:
    std::shared_ptr<CUICanvas> editor_canvas;
    std::shared_ptr<CUIComponent> selected_UI = nullptr;
    char save_path[256] = "UI_Layout.json";

    // UIScene은 Shader를 따로 가짐
    std::unordered_map<std::string, std::shared_ptr<CShader>>	shaders{};
    std::map<std::string, std::unique_ptr<IRenderer>> renderers;	// shader에 버퍼 설정하는 멤버 변수(rendering 담당)
};

