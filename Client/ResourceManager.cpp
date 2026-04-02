#include "stdafx.h"
#include "ResourceManager.h"
#include "ImGuiManager.h"

void CResourceManager::LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // 타이틀 씬 관련 텍스처 로드
    LoadTitleSceneTextures(device, cmdQueue);

    // 게임 씬 관련 텍스처 로드
    LoadGameSceneTextures(device, cmdQueue);
}

void CResourceManager::LoadTitleSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "bg",     L"../Resource/TitleScene/Background.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "title",  L"../Resource/TitleScene/UNDEAD.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "single", L"../Resource/TitleScene/SinglePlayBtn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "multi",  L"../Resource/TitleScene/MultiplayBtn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "exit",   L"../Resource/TitleScene/ExitBtn.png");
}

void CResourceManager::LoadGameSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{

}
