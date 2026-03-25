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
		std::shared_ptr<CShader> shader = std::make_unique<CBillboardShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("billboard", std::move(shader));
	}
	{
		// UI
		std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("ui", std::move(shader));
	}

	// 1. 캔버스 생성
	auto mainCanvas = CUIManager::GetInstance().CreateCanvas();

	// 2. 체력바 배경 추가 (예시)
	auto hpBarBg = std::make_shared<CUIImage>();
	hpBarBg->SetSize({ 400.0f, 40.0f });
	hpBarBg->SetRelativePos({ 0.0f, 450.0f }); // 화면 하단 쪽

	mainCanvas->AddChild(hpBarBg);

	auto uiText = std::make_shared<CUIText>();
	uiText->SetSize({ 400.0f, 40.0f });
	uiText->SetRelativePos({ 0.0f, 300.0f }); // 화면 하단 쪽

	mainCanvas->AddChild(uiText);

	auto uibutton = std::make_shared<CUIButton>();
	uibutton->SetSize({ 400.0f, 40.0f });
	uibutton->SetRelativePos({ 0.0f, 100 }); // 화면 하단 쪽

	mainCanvas->AddChild(uibutton);

	factory->GetMaterial(shaders["ui"]->GetHeapManager(), "white");	// 인덱스 0에 생성하기 위해 먼저 생성

	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(staticHeapManager);
		auto obj = factory->CreatePlayer(staticHeapManager);
		obj->SetPosition(3, 0, 3);

		auto billboard = std::make_shared<CUIBillboard>(obj.get());
		std::shared_ptr<CMaterialComponent> m = std::make_shared<CMaterialComponent>();
		m->SetMaterial(factory->GetMaterial(shaders["billboard"]->GetHeapManager(), "white"));
		billboard->SetMaterial(m);

		mainCanvas->AddChild(billboard);
		objects.push_back(std::move(obj));
	}

	{
		auto instRenderer = std::make_unique<CInstRenderer>();
		instRenderer->Initialize(GET_DEVICE, objects.size());
		renderers["inst"] = std::move(instRenderer);

		auto uiRenderer = std::make_unique<CUIRenderer>();
		uiRenderer->Initialize(GET_DEVICE, 100);
		renderers["ui"] = std::move(uiRenderer);

		auto bbRenderer = std::make_unique<CBillboardRenderer>();
		bbRenderer->Initialize(GET_DEVICE, 500);
		renderers["billboard"] = std::move(bbRenderer);

		// 20부터 font용(아직 제한X)
		CDescriptorHeapManager* heap = shaders["ui"]->GetHeapManager();
		auto textRenderer = std::make_unique<CTextRenderer>();
		textRenderer->Initialize(GET_DEVICE, GET_CMD_QUEUE, heap->GetCPUHandle(20), heap->GetGPUHandle(20));
		renderers["text"] = std::move(textRenderer);
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		my_player->SetPosition(0.0f, 2.0f, 0.0f);
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
	CUIManager::GetInstance().Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
	}
}

void CGameScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera) camera->SetViewportsAndScissorRects(commandList);

	// Collect Phase
	// 3D 객체들 수집
	for (const auto& obj : objects) {
		auto it = renderers.find(obj->GetShader());
		if (it != renderers.end()) {
			obj->OnCollect(it->second.get());
		}
	}

	// 플레이어 수집
	if (my_player) {
		auto it = renderers.find(my_player->GetShader());
		if (it != renderers.end()) {
			my_player->OnCollect(it->second.get());
		}
	}

	// UI 매니저 수집
	CUIManager::GetInstance().Collect(renderers);


	// 4. 일반 2D UI 렌더링
	// Draw Phase
	for (const auto& [name, pShader] : shaders) {
		pShader->RenderBegin(commandList);

		if (name == "billboard") {
			camera->UpdateShaderVariablesBillBoard(commandList);
		}
		else if (name == "ui") {
			camera->UpdateShaderVariables(commandList, true);
		}
		else {
			camera->UpdateShaderVariables(commandList, false);
		}

		// 2. 광원 처리 (UI와 빌보드는 보통 자체 발광이므로 라이트를 끕니다)
		bool useLight = (name != "ui" && name != "billboard");
		if (light && useLight) {
			light->Render(commandList);
		}

		// 3. 실제 그리기 (인스턴싱 드로우)
		auto it = renderers.find(name);
		if (it != renderers.end()) {
			it->second->Render(commandList);
			if (name == "ui") {
				renderers["text"]->Render(commandList);
			}
		}

		pShader->RenderEnd(commandList);
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