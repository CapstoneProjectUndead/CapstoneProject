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
	std::shared_ptr<CMyPlayer> myPlayer = active_scene->GetMyPlayer();

	// 기존 씬에서 정리할게 있으면 여기서 처리
	active_scene->Exit();
	
	// 바꾸고자하는 씬으로 active_scene을 변경
	active_scene = scenes[(UINT)type].get();

	// 해당 씬에 플레이어 셋팅
	active_scene->SetPlayer(myPlayer);

	// 해당 씬에서 할 게 있으면 여기서 처리
	active_scene->Enter();
}