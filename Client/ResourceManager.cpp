#include "stdafx.h"
#include "ResourceManager.h"
#include "ImGuiManager.h"
#include "SoundManager.h"

void CResourceManager::LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // 타이틀 씬 관련 텍스처 로드
    LoadTitleSceneTextures(device, cmdQueue);

    // 게임 씬 관련 텍스처 로드
    LoadGameSceneTextures(device, cmdQueue);

    // 사운드 리소스 로드
    LoadSounds();
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
    // 장비 아이템 이미지
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_shovel",         L"../Modeling/item/image/Equip_shovel.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_ax",             L"../Modeling/item/image/Equip_ax.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_pick",           L"../Modeling/item/image/Equip_pick.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_bat",            L"../Modeling/item/image/Equip_bat.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_squeaky_hammer", L"../Modeling/item/image/Equip_squeaky_hammer.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_magicwand",      L"../Modeling/item/image/Equip_magicwand.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_toysword",       L"../Modeling/item/image/Equip_toysword.png");

    // 음식/소비 아이템 이미지
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_meat",             L"../Modeling/item/image/Food_meat.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_frozen_dumplings", L"../Modeling/item/image/Food_frozen_dumplings.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_broccoli",         L"../Modeling/item/image/Food_broccoli.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_durian",           L"../Modeling/item/image/Food_durian.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_banana",           L"../Modeling/item/image/Food_banana.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_snack",            L"../Modeling/item/image/Food_snack.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_mandarin",         L"../Modeling/item/image/Food_mandarin.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_potato",           L"../Modeling/item/image/Food_potato.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_chocolate",        L"../Modeling/item/image/Food_chocolate.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_candy",            L"../Modeling/item/image/Food_candy.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_orange_juice",     L"../Modeling/item/image/Food_orange_juice.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_churros",          L"../Modeling/item/image/Food_churros.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_candyfluff",       L"../Modeling/item/image/Food_candyfluff.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_icecream",         L"../Modeling/item/image/Food_icecream.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_melonpan",         L"../Modeling/item/image/Food_melonpan.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_Nachos",           L"../Modeling/item/image/Food_Nachos.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_jellybean",        L"../Modeling/item/image/Food_jellybean.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_drumstick",        L"../Modeling/item/image/Food_drumstick.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_cheese",           L"../Modeling/item/image/Food_cheese.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_yogurt",           L"../Modeling/item/image/Food_yogurt.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_cocoa",            L"../Modeling/item/image/Food_cocoa.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_cake",             L"../Modeling/item/image/Food_cake.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_snackbox",         L"../Modeling/item/image/Food_snackbox.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_pizza",            L"../Modeling/item/image/Food_pizza.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_fish",             L"../Modeling/item/image/Food_fish.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Food_lunch",            L"../Modeling/item/image/Food_lunch.png");
}

void CResourceManager::LoadSounds()
{
    CSoundManager::GetInstance().LoadSound(SOUND_ID::button01a, "../Resource/Sound/button01a.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::select09, "../Resource/Sound/select09.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::jump12,   "../Resource/Sound/jump12.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::damaged1,   "../Resource/Sound/damaged1.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::ghost_attack,   "../Resource/Sound/ghost_attack.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::crude_laughter,   "../Resource/Sound/crude_laughter.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::devil_laugh1,   "../Resource/Sound/devil_laugh1.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::flying_pan,   "../Resource/Sound/flying_pan.mp3");
}
