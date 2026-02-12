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
    // Title Scene 하나 Update
    CSceneManager::GetInstance().Update(elapsedTime);

    // Room 안에 있는 씬들을 Update
    CRoomManager::GetInstance().Update(elapsedTime);
}

void CGameFramework::SendResults()
{
    // Title Scene 하나 SendResults
    CSceneManager::GetInstance().SendResults();

    // Room 안에 있는 씬들을 SendResults
    CRoomManager::GetInstance().SendResults();
}
