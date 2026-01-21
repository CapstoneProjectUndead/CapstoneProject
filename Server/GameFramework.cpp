#include "pch.h"
#include "GameFramework.h"
#include "SceneManager.h"

CGameFramework::CGameFramework()
{

}

CGameFramework::~CGameFramework()
{

}

void CGameFramework::Init()
{
    CSceneManager::GetInstance().Initialize();
}

void CGameFramework::Update()
{
    CSceneManager::GetInstance().Update();
}
