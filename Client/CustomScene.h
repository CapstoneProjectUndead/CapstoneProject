#pragma once
#include "Scene.h"

class CCustomScene : public CScene
{
public:
	CCustomScene() : CScene(SCENE_TYPE::CUSTOMS) {};
	void Initialize() override;
	void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;

	void Enter() override;
	void Exit() override;
	bool IsUIInputEnabled() override;

	void DrawUI() override;
	void DrawCustomizingWindow();
public:
	// Scene 클래스 멤버 변수 혹은 정적 변수
	int body_idx{};
	int eyes_idx{};
	int mouth_idx{};

	const int max_model = 3;
};

