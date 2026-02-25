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

private:
	void DrawCustomizingWindow();

	void DrawLoadingPopUp();
	void DrawLoadingPopUpResult();

	void ShowResultPopup(bool is_success, const std::string& msg);
	void CloseResultPopup();

	void StartLoading(LoadingType type) { loading_type = type; }
	void StopLoading() { loading_type = LoadingType::None; }

public:
	// 서버 패킷 관련 처리 함수들
	void Handle_S_Custom_Select(std::shared_ptr<Session> session, S_CustomSelect& pkt);

public:
	// Scene 클래스 멤버 변수 혹은 정적 변수
	int body_idx{};
	int eyes_idx{};
	int mouth_idx{};

	const int max_model = 3;

private:
	LoadingType  loading_type = LoadingType::None;
	ActionResult pop_up_result;
};

