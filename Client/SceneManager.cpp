#include "stdafx.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Timer.h"


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