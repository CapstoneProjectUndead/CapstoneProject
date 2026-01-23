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

void CGameFramework::Update(float elapsedTime)
{
    CSceneManager::GetInstance().Update(elapsedTime);
}

void CGameFramework::SendResults()
{
}
