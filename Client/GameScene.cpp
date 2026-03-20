#include "stdafx.h"
#include "GameScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"
#include "SceneManager.h"
#include "MeshRenderer.h"
#include "Renderers.h"

#include "UIComponent.h"

CGameScene::CGameScene()
	: CScene(SCENE_TYPE::GAME)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("skinning", std::move(shader));
	}
	{
		// inst
		std::shared_ptr<CShader> shader = std::make_unique<CInstShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("inst", std::move(shader));
	}
	{
		// UI
		std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("ui", std::move(shader));
	}

	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(staticHeapManager);
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		my_player->SetPosition(0.0f, 2.0f, 0.0f);
		std::shared_ptr<CUIComponent> ui = std::make_shared<CUIComponent>();
		my_player->SetComponent(ui);
		CUIRenderer::GetInstance().Initialize(device, commandList, 1);
	}

	if (!camera) {
		camera = std::make_shared<CCamera>();
		camera->SetTarget(my_player.get());
		camera->Initialize(device, commandList);
	}

	// light 생성
	if (!light) {
		light = std::make_unique<CLightManager>();
		light->Initialize(device, commandList);
	}
}

void CGameScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
	}
}

void CGameScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (const auto& shader : shaders) {
		if (shader.first == "ui") continue;
		shader.second->RenderBegin(commandList);

		camera->UpdateShaderVariables(commandList);

		if (light) {
			light->Render(commandList);
		}

		for (const auto& obj : objects) {
			if (shader.first == obj->GetShader()) {
				shader.second->Render(commandList, obj.get());
			}
		}

		if (my_player) {
			if (shader.first == my_player->GetShader()) {
				shader.second->Render(commandList, my_player.get());
			}
		}

		if (shader.first == "inst")
			CInstRenderer::GetInstance().Render(commandList);

		shader.second->RenderEnd(commandList);
	}

	auto uiShader = shaders.find("ui");
	if (uiShader != shaders.end()) {
		uiShader->second->RenderBegin(commandList); // 여기서 RootSig, PSO 설정
		camera->UpdateShaderVariables(commandList, true); // Ortho 행렬 전달

		// 중요: UI용 Descriptor Heap을 여기서 다시 한번 세팅!
		CUIRenderer::GetInstance().Render(commandList);

		uiShader->second->RenderEnd(commandList);
	}

}

void CGameScene::DrawUI()
{
}

bool CGameScene::IsUIInputEnabled()
{
	bool state = true;

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene);

	if (scene->GetSceneType() == SCENE_TYPE::GAME)
		state = false;

	return state;
}

void CGameScene::Enter()
{
	CScene::Enter();

	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::GAME);
		camera->SetTarget(my_player.get());
	}
}