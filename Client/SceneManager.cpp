#include "stdafx.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Timer.h"
#include "MyPlayer.h"


void CSceneManager::Update()
{
	if (active_scene) {
		active_scene->Update(CTimer::GetInstance().GetTimeElapsed());
	}
}

void CSceneManager::Render(ID3D12GraphicsCommandList* commandList)
{
	if (active_scene) {
		active_scene->Render(commandList);
	}
}

void CSceneManager::ChangeScene(SCENE_TYPE type)
{
	// 기존 씬에 있던 내 플레이어를 찾아온다.
	// Title Scene에서 시작하는 경우에는 없을 수 있기 때문에, 반드시 null 체크를 해야한다. 
	std::shared_ptr<CMyPlayer> myPlayer;
	if (active_scene->GetMyPlayer())
		myPlayer = active_scene->GetMyPlayer();

	// 기존 씬의 shader Set
	std::unordered_map<std::string, std::shared_ptr<CShader>> newShader;
	auto& shaders = active_scene->GetShaders();
	if (!shaders.empty()) {
		newShader = shaders;
	}

	// 기존 씬에서 정리할게 있으면 여기서 처리
	active_scene->Exit();
	
	// 바꾸고자하는 씬으로 active_scene을 변경
	active_scene = scenes[(UINT)type].get();

	// 해당 씬에 플레이어 셋팅
	if (myPlayer && active_scene->GetSceneType() != SCENE_TYPE::TITLE)
		active_scene->SetPlayer(myPlayer);

	if (!newShader.empty())
		active_scene->SetShaders(newShader);

	// 해당 씬에서 할 게 있으면 여기서 처리
	active_scene->Enter();
}
