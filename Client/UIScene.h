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
    virtual void Update(float dt) override;
	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Render(ID3D12GraphicsCommandList*) override;

	virtual void DrawUI() override;
	virtual bool IsUIInputEnabled() override;
private:
    void RenderHierarchyWindow();
    void DrawHierarchyNode(std::shared_ptr<CUIComponent> node);
    std::string OpenFileDialog();
    void RenderInspectorWindow();
    void RenderToolbar();
    void HandleUIDragging();
    void DeleteSelectedUI();
    std::shared_ptr<CUIComponent> FindParent(std::shared_ptr<CUIComponent> root, std::shared_ptr<CUIComponent> target);
    void CopySelectedUI();
    void PasteUI();
private:
    std::shared_ptr<CUICanvas> editor_canvas;
    std::shared_ptr<CUIComponent> selected_UI = nullptr;
    char save_path[256] = "../Modeling/UI/UI_Layout.json";
    json clipboard;
};

