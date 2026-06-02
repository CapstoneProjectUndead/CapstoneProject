#include "stdafx.h"
#include "ResourceManager.h"
#include "ImGuiManager.h"
#include "SoundManager.h"

void CResourceManager::LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // 타이틀 씬 관련 텍스처 로드
    LoadTitleSceneTextures(device, cmdQueue);

    // 커스텀 씬 관련 텍스쳐 로드
    LoadCustomSceneTextures(device, cmdQueue);

    // 로비 씬 관련 텍스쳐 로드
    LoadLobbySceneTextures(device, cmdQueue);

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
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "login",   L"../Resource/TitleScene/LogIn_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "register",   L"../Resource/TitleScene/Register_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "goback",   L"../Resource/TitleScene/GoBack_Bnt.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "search",   L"../Resource/TitleScene/Search_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "logout",   L"../Resource/TitleScene/LogOut_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "createroom",   L"../Resource/TitleScene/Room_Create_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "enterroom",   L"../Resource/TitleScene/Entry_Btn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "refresh",   L"../Resource/TitleScene/Refresh_Btn.png");
}

void CResourceManager::LoadCustomSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "portrait_dog", L"../Resource/CustomScene/Dog.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "portrait_cat", L"../Resource/CustomScene/Cat.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "portrait_bunny", L"../Resource/CustomScene/Bunny.png");
}

void CResourceManager::LoadLobbySceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "sold_out", L"../Resource/LobbyScene/sold_out.png");
}

void CResourceManager::LoadGameSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // 상대 플레이어 상태 아이콘 (사망/빈사/빙의)
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "status_death",       L"../Resource/GameScene/death.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "status_almost_dead", L"../Resource/GameScene/almost_dead.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "status_possessed",   L"../Resource/GameScene/possessed.png");

    // 장비 아이템 이미지
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_shovel",         L"../Modeling/item/image/Equip_shovel.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_ax",             L"../Modeling/item/image/Equip_ax.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_pick",           L"../Modeling/item/image/Equip_pick.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_bat",            L"../Modeling/item/image/Equip_bat.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_squeaky_hammer", L"../Modeling/item/image/Equip_squeaky_hammer.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_magicwand",      L"../Modeling/item/image/Equip_magicwand.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_toysword",       L"../Modeling/item/image/Equip_toysword.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Equip_ghostspray",       L"../Modeling/item/image/Equip_ghostspray.png");

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

    // 기타 아이템 이미지
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_balloon", L"../Modeling/item/image/Etc_balloon.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_bananapeel", L"../Modeling/item/image/Etc_bananapeel.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_cobweb", L"../Modeling/item/image/Etc_cobweb.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_memo", L"../Modeling/item/image/Etc_memo.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_oldsocks", L"../Modeling/item/image/Etc_oldsocks.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_pail", L"../Modeling/item/image/Etc_pail.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_piggybank", L"../Modeling/item/image/Etc_piggybank.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_slime", L"../Modeling/item/image/Etc_slime.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_trap", L"../Modeling/item/image/Etc_trap.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "Etc_vuvuzela", L"../Modeling/item/image/Etc_vuvuzela.png");

    // 보물 아이템 이미지 
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Rusty_Coin",             L"../Modeling/treasure/image/C_Rusty_Coin.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Glass_Marble",           L"../Modeling/treasure/image/C_Glass_Marble.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Old_Spoon",              L"../Modeling/treasure/image/C_Old_Spoon.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Empty_Bottle",           L"../Modeling/treasure/image/C_Empty_Bottle.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Seashell",               L"../Modeling/treasure/image/C_Seashell.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Scrap_Map",              L"../Modeling/treasure/image/C_Scrap_Map.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Wooden_Doll",            L"../Modeling/treasure/image/C_Wooden_Doll.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Paper_Plane",            L"../Modeling/treasure/image/C_Paper_Plane.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Bottle_Cap",             L"../Modeling/treasure/image/C_Bottle_Cap.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Tennis_Ball",            L"../Modeling/treasure/image/C_Tennis_Ball.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Broken_Washing_Machine", L"../Modeling/treasure/image/C_Broken_Washing_Machine.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Fish_Bone",              L"../Modeling/treasure/image/C_Fish_Bone.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Wooden_Button",          L"../Modeling/treasure/image/C_Wooden_Button.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Empty_Can",              L"../Modeling/treasure/image/C_Empty_Can.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Brick",                  L"../Modeling/treasure/image/C_Brick.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Cracked_Plate",          L"../Modeling/treasure/image/C_Cracked_Plate.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Old_Boot",               L"../Modeling/treasure/image/C_Old_Boot.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "C_Broken_Clock",           L"../Modeling/treasure/image/C_Broken_Clock.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Copper_Ring",            L"../Modeling/treasure/image/U_Copper_Ring.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Silk_Handkerchief",      L"../Modeling/treasure/image/U_Silk_Handkerchief.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Pearl_Earring",          L"../Modeling/treasure/image/U_Pearl_Earring.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Brass_Canddlestick",     L"../Modeling/treasure/image/U_Brass_Canddlestick.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Ancient_Clay_Tablet",    L"../Modeling/treasure/image/U_Ancient_Clay_Tablet.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Bronze_Mirror",          L"../Modeling/treasure/image/U_Bronze_Mirror.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Amethyst_Shard",         L"../Modeling/treasure/image/U_Amethyst_Shard.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Frill_Ribbon",           L"../Modeling/treasure/image/U_Frill_Ribbon.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Leather_Belt",           L"../Modeling/treasure/image/U_Leather_Belt.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Ceramic_Vase",           L"../Modeling/treasure/image/U_Ceramic_Vase.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Golden Dice",            L"../Modeling/treasure/image/U_Golden Dice.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Boxing_Glove",           L"../Modeling/treasure/image/U_Boxing_Glove.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Car_Key",                L"../Modeling/treasure/image/U_Car_Key.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "U_Luxury_Bag",             L"../Modeling/treasure/image/U_Luxury_Bag.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Silver_Ring",            L"../Modeling/treasure/image/R_Silver_Ring.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Emerald_Brooch",         L"../Modeling/treasure/image/R_Emerald_Brooch.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Crystal_Skull",          L"../Modeling/treasure/image/R_Crystal_Skull.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Viking_Helmet",          L"../Modeling/treasure/image/R_Viking_Helmet.png");

    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "L_Phoenix_Feather",          L"../Modeling/treasure/image/L_Phoenix_Feather.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "L_Philosopher_Stone",          L"../Modeling/treasure/image/L_Philosopher_Stone.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "L_Imperial_crown",          L"../Modeling/treasure/image/L_Imperial_crown.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "L_Heart_of_Ocean",          L"../Modeling/treasure/image/L_Heart_of_Ocean.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "L_Excalibur",          L"../Modeling/treasure/image/L_Excalibur.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Unicorn_Horn",          L"../Modeling/treasure/image/E_Unicorn_Horn.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Meteorite_Shard", L"../Modeling/treasure/image/E_Meteorite_Shard.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Golden_Chalice", L"../Modeling/treasure/image/E_Golden_Chalice.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Dinosaur_Egg_Fossil", L"../Modeling/treasure/image/E_Dinosaur_Egg_Fossil.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Diamond_Ring", L"../Modeling/treasure/image/E_Diamond_Ring.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Cursed_Totem", L"../Modeling/treasure/image/E_Cursed_Totem.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "E_Chest_of_Gold", L"../Modeling/treasure/image/E_Chest_of_Gold.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Treasure_Map", L"../Modeling/treasure/image/R_Treasure_Map.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Samurai_Katana", L"../Modeling/treasure/image/R_Samurai_Katana.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Knight_Armor", L"../Modeling/treasure/image/R_Knight_Armor.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Jade_Bangle", L"../Modeling/treasure/image/R_Jade_Bangle.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "R_Antique_Typewriter", L"../Modeling/treasure/image/R_Antique_Typewriter.png");

    // 빙의 해제 UI
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "ghost_icon", L"../Resource/GameScene/ghost.png");

    // 구조 UI
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "heal_icon", L"../Resource/GameScene/heal.png");
    CImGuiManager::GetInstance().LoadTexture(device, cmdQueue, "giveup", L"../Resource/GameScene/giveup_btn.png");
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
    CSoundManager::GetInstance().LoadSound(SOUND_ID::jab,   "../Resource/Sound/jab.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::ghost_spray,   "../Resource/Sound/ghost_spray.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::devil_scared1,   "../Resource/Sound/devil_scared1.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::surprising_girl,   "../Resource/Sound/surprising_girl.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::girl_flee,   "../Resource/Sound/girl_flee.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::ridicule,   "../Resource/Sound/ridicule.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::swing2,   "../Resource/Sound/swing2.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::sword,   "../Resource/Sound/sword.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::dog_bark,   "../Resource/Sound/dog_bark.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::dog_attack,   "../Resource/Sound/dog_attack.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::dog_moan,   "../Resource/Sound/dog_moan.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::dog_howling,   "../Resource/Sound/dog_howling.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::clock_alarm,   "../Resource/Sound/clock_alarm.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::pick_up,   "../Resource/Sound/pick_up.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::warning_bell,   "../Resource/Sound/warning_bell.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::bare_hand_dig,   "../Resource/Sound/bare_hand_dig.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::Settlement,   "../Resource/Sound/Settlement.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::TaDa,   "../Resource/Sound/TaDa.mp3");
    CSoundManager::GetInstance().LoadSound(SOUND_ID::Return,   "../Resource/Sound/Return.mp3");
}
