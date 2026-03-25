#pragma once
#include "Scene.h"

// 실제 게임에서 사용X
class CUIScene : public CScene {
public:
	CUIScene();
	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;

	virtual void DrawUI() override;
	virtual bool IsUIInputEnabled() override;
};

