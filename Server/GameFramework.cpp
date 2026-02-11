#include "pch.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "RoomManager.h"

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

void CGameFramework::Update(const float elapsedTime)
{
    CSceneManager::GetInstance().Update(elapsedTime);
    CRoomManager::GetInstance().Update(elapsedTime);
}

void CGameFramework::SendResults()
{
    CSceneManager::GetInstance().SendResults();
    CRoomManager::GetInstance().SendResults();
}
