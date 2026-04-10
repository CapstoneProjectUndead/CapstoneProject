#include "pch.h"
// Server쪽 CGameFramework
#include "GameFramework.h"
#include "SceneManager.h"
#include "RoomManager.h"
#include "ItemFactory.h"
#include <filesystem>

CGameFramework::CGameFramework()
{

}

CGameFramework::~CGameFramework()
{

}

void CGameFramework::Init()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path jsonPath = std::filesystem::path(exePath).parent_path() / "Data/items.json";
    ItemFactory::LoadFromJson(jsonPath.string());

    CSceneManager::GetInstance().Initialize();
    CRoomManager::GetInstance().Initialize();
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
