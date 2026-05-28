#include "stdafx.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Timer.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "GameFramework.h"
#include "AnimationManager.h"
#include "KeyManager.h"
#include "ShadowMap.h"
#include "SkyBox.h"

void CSceneManager::Init(ID3D12Device* device)
{
	// shader
	shaders.resize(EShaderName::Count);
	{
		// shadow
		std::shared_ptr<CShader> shader = std::make_unique<CShadowShader>();
		shader->CreateShader(device);
		shaders[EShaderName::Shadow] = std::move(shader);
	}
	{
		// twoside(D3D12_CULL_MODE_NONE)
		std::shared_ptr<CShader> shader = std::make_unique<CTwoSideShader>();
		shader->CreateShader(device);
		shaders[EShaderName::TwoSide] = std::move(shader);
	}
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(device);
		{
			auto skinningHeapManager = shader->GetHeapManager();
			CAnimationManager::GetInstance().Initialize("../Modeling/undead_char.bin", "../Modeling/undead_ani_baking.bin", OBJECT_TYPE::PLAYER, NULL);
			CAnimationManager::GetInstance().Initialize("../Modeling/Human_monster.bin", "../Modeling/Human_monster_ani.bin", OBJECT_TYPE::MONSTER, static_cast<uint8_t>(MON_TYPE::HUMAN_MONSTER));
			CAnimationManager::GetInstance().Initialize("../Modeling/Ghost3.bin", "../Modeling/Ghost3_ani.bin", OBJECT_TYPE::MONSTER, static_cast<uint8_t>(MON_TYPE::GHOST));
			CAnimationManager::GetInstance().Initialize("../Modeling/Dog.bin", "../Modeling/Dog_ani.bin", OBJECT_TYPE::MONSTER, static_cast<uint8_t>(MON_TYPE::ANIMAL_MONSTER));
			CAnimationManager::GetInstance().CreateAnimationTexture(device, GET_CMD_LIST, skinningHeapManager->GetSRVCPUHandle(skinningHeapManager->GetSRVHeap().Allocate()));
			CAnimationManager::GetInstance().CreateMaskBuffer(device, GET_CMD_LIST, skinningHeapManager->GetSRVCPUHandle(skinningHeapManager->GetSRVHeap().Allocate()));
		}
		shaders[EShaderName::Skinning] = std::move(shader);
	}
	{
		// billboard(ui용)
		std::shared_ptr<CShader> shader = std::make_unique<CBillboardShader>();
		shader->CreateShader(device);
		shaders[EShaderName::Billboard] = std::move(shader);
	}
	{
		// UI
		std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
		shader->CreateShader(device);
		shaders[EShaderName::UI] = std::move(shader);
	}
	{
		// SkyBox
		std::shared_ptr<CShader> shader = std::make_unique<CSkyBoxShader>();
		shader->CreateShader(device);
		shaders[EShaderName::SkyBox] = std::move(shader);
	}

	// renderer
	renderers.resize(EShaderName::Count);
	{
		auto shadowRenderer = std::make_unique<CAniRenderer>();
		shadowRenderer->Initialize(device, 100);
		renderers[EShaderName::Shadow] = std::move(shadowRenderer);

		auto twiSideRenderer = std::make_unique<CInstRenderer>();
		twiSideRenderer->Initialize(device, 100);
		renderers[EShaderName::TwoSide] = std::move(twiSideRenderer);

		auto aniRenderer = std::make_unique<CAniRenderer>();
		aniRenderer->Initialize(device, 1500);
		renderers[EShaderName::Skinning] = std::move(aniRenderer);

		auto uiRenderer = std::make_unique<CUIRenderer>();
		uiRenderer->Initialize(device, 100);
		renderers[EShaderName::UI] = std::move(uiRenderer);

		auto bbRenderer = std::make_unique<CBillboardRenderer>();
		bbRenderer->Initialize(device, 500);
		renderers[EShaderName::Billboard] = std::move(bbRenderer);

		// 20부터 font용(아직 제한X)
		CDescriptorHeapManager* heap = shaders[EShaderName::UI]->GetHeapManager();
		auto textRenderer = std::make_unique<CTextRenderer>();
		textRenderer->Initialize(device, GET_CMD_QUEUE, heap->GetSRVCPUHandle(20), heap->GetSRVGPUHandle(20));
		renderers[EShaderName::Text] = std::move(textRenderer);
	}

	// shadow map
	{
		shadow_map = std::make_shared<CShadowMap>(GET_DEVICE, 4096, 4096);
		auto skinHeap = shaders[EShaderName::Skinning]->GetHeapManager();
		auto twosideHeap = shaders[EShaderName::TwoSide]->GetHeapManager();
		auto shadowHeap = shaders[EShaderName::Shadow]->GetHeapManager();

		shadow_map->CreateDescriptors(
			shadowHeap->GetSRVCPUHandle(DescriptorSlot::ShadowMapIdx),
			shadowHeap->GetSRVGPUHandle(DescriptorSlot::ShadowMapIdx),
			shadowHeap->GetDSVCPUHandle(0)
		);

		shadow_map->CreateSRV(skinHeap->GetSRVCPUHandle(DescriptorSlot::ShadowMapIdx));
		shadow_map->CreateSRV(twosideHeap->GetSRVCPUHandle(DescriptorSlot::ShadowMapIdx));
	}

	// skyBox
	skybox = std::make_shared<CSkyBox>();
	skybox->Initialize(device, GET_CMD_LIST, shaders[EShaderName::SkyBox]->GetHeapManager());
}

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
	// Title Scene에서 시작하는 경우에는 없을 수 있기 때문에, 반드시 null 체크를 해야한다. 
	std::shared_ptr<CMyPlayer> myPlayer;
	if (active_scene->GetMyPlayer())
		myPlayer = active_scene->GetMyPlayer();

	// 기존 씬에서 정리할게 있으면 여기서 처리
	active_scene->Exit();
	
	// 바꾸고자하는 씬으로 active_scene을 변경
	active_scene = scenes[(UINT)type].get();

	// 해당 씬에 플레이어 셋팅
	if (myPlayer && active_scene->GetSceneType() != SCENE_TYPE::TITLE)
		active_scene->SetPlayer(myPlayer);

	if (type == SCENE_TYPE::LOBBY || type == SCENE_TYPE::GAME) {
		CKeyManager::GetInstance().SetMouseMode(true);
	}
	else {
		CKeyManager::GetInstance().SetMouseMode(false);
	}

	// 해당 씬에서 할 게 있으면 여기서 처리
	active_scene->Enter();
}
