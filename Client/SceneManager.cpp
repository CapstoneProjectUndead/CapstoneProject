#include "stdafx.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Timer.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "GameFramework.h"
#include "AnimationManager.h"

void CSceneManager::Init(ID3D12Device* device)
{
	// shader
	{
		// inst
		std::shared_ptr<CShader> shader = std::make_unique<CInstShader>();
		shader->CreateShader(device);
		shaders.emplace("inst", std::move(shader));
	}
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(device);
		{
			auto skinningHeapManager = shader->GetHeapManager();
			CAnimationManager::GetInstance().Initialize("../Modeling/undead_char.bin", "../Modeling/undead_ani_baking.bin");
			CAnimationManager::GetInstance().CreateAnimationTexture(device, GET_CMD_LIST, skinningHeapManager->GetCPUHandle(skinningHeapManager->Allocate()));
			CAnimationManager::GetInstance().CreateMaskBuffer(device, GET_CMD_LIST, skinningHeapManager->GetCPUHandle(skinningHeapManager->Allocate()));
		}
		shaders.emplace("skinning", std::move(shader));
	}
	{
		// billboard(ui용)
		std::shared_ptr<CShader> shader = std::make_unique<CBillboardShader>();
		shader->CreateShader(device);
		shaders.emplace("billboard", std::move(shader));
	}
	{
		// UI
		std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
		shader->CreateShader(device);
		shaders.emplace("ui", std::move(shader));
	}

	// renderer
	{
		auto instRenderer = std::make_unique<CInstRenderer>();
		instRenderer->Initialize(device, 5000);
		renderers["inst"] = std::move(instRenderer);

		auto aniRenderer = std::make_unique<CAniRenderer>();
		aniRenderer->Initialize(device, 100);
		renderers["skinning"] = std::move(aniRenderer);

		auto uiRenderer = std::make_unique<CUIRenderer>();
		uiRenderer->Initialize(device, 100);
		renderers["ui"] = std::move(uiRenderer);

		auto bbRenderer = std::make_unique<CBillboardRenderer>();
		bbRenderer->Initialize(device, 500);
		renderers["billboard"] = std::move(bbRenderer);

		// 20부터 font용(아직 제한X)
		CDescriptorHeapManager* heap = shaders["ui"]->GetHeapManager();
		auto textRenderer = std::make_unique<CTextRenderer>();
		textRenderer->Initialize(device, GET_CMD_QUEUE, heap->GetCPUHandle(20), heap->GetGPUHandle(20));
		renderers["text"] = std::move(textRenderer);
	}
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

	// 해당 씬에서 할 게 있으면 여기서 처리
	active_scene->Enter();
}
