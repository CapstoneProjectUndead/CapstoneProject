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
		// cubeShadow
		std::shared_ptr<CShader> cubeShader = std::make_unique<CCubeShadowShader>();
		cubeShader->CreateShader(device);
		shaders[EShaderName::CubeShadow] = std::move(cubeShader);
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
	{
		// Deferred
		std::shared_ptr<CShader> shader = std::make_unique<CDeferredShader>();
		shader->CreateShader(device);
		shaders[EShaderName::Deferred] = std::move(shader);
	}
	{
		// SSAO
		std::shared_ptr<CShader> shader = std::make_unique<CAOShader>();
		shader->CreateShader(device);
		shaders[EShaderName::SSAO] = std::move(shader);
	}

	// renderer
	renderers.resize(EShaderName::Count);
	{
		auto shadowRenderer = std::make_unique<CShadowRenderer>();
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

		CDescriptorHeapManager* heap = shaders[EShaderName::UI]->GetHeapManager();
		auto textRenderer = std::make_unique<CTextRenderer>();
		textRenderer->Initialize(device, GET_CMD_QUEUE, heap->GetSRVCPUHandle(20), heap->GetSRVGPUHandle(20));
		renderers[EShaderName::Text] = std::move(textRenderer);
	}

	auto deferredLightingHeap = shaders[EShaderName::Deferred]->GetHeapManager();
	// directional light shadow map
	{
		dir_shadow_map = std::make_shared<CShadowMap>(GET_DEVICE, 4096, 4096);
		auto shadowHeap = shaders[EShaderName::Shadow]->GetHeapManager();

		dir_shadow_map->CreateDescriptors(
			shadowHeap->GetSRVCPUHandle(0),
			shadowHeap->GetSRVGPUHandle(0),
			shadowHeap->GetDSVCPUHandle(0)
		);
		dir_shadow_map->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::ShadowMapIdx));
	}
	// dot light shadow map
	{
		cube_shadow_map = std::make_shared<CCubeShadowMap>(GET_DEVICE, 1024, 1024);
		auto shadowHeap = shaders[EShaderName::CubeShadow]->GetHeapManager();

		cube_shadow_map->CreateDescriptors(
			shadowHeap->GetSRVCPUHandle(0),
			shadowHeap->GetSRVGPUHandle(0),
			shadowHeap->GetDSVCPUHandle(0)
		);
		cube_shadow_map->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::CubeMapIdx));
	}
	// skyBox
	{
		skybox = std::make_shared<CSkyBox>();
		skybox->Initialize(device, GET_CMD_LIST, shaders[EShaderName::SkyBox]->GetHeapManager());
		skybox->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::SkyboxMapIdx));
	}

	// G-Buffer 셋업 및 최종 조명 합성 바인딩 데이터 연결
	{
		buffer_color = std::make_unique<CGBufferTarget>(device, GET_CLIENT_WIDTH, GET_CLIENT_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
		buffer_normal = std::make_unique<CGBufferTarget>(device, GET_CLIENT_WIDTH, GET_CLIENT_HEIGHT, DXGI_FORMAT_R16G16B16A16_FLOAT);
		buffer_ssao = std::make_unique<CRenderTarget>(device, GET_CLIENT_WIDTH, GET_CLIENT_HEIGHT, DXGI_FORMAT_R8_UNORM);

		auto ssaoHeap = shaders[EShaderName::SSAO]->GetHeapManager();
		buffer_color->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::GBufferColorIdx));
		buffer_normal->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::GBufferNormalIdx));
		buffer_normal->CreateSRV(ssaoHeap->GetSRVCPUHandle(DescriptorSlot::GBufferNormalIdx));
		buffer_ssao->CreateSRV(deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::AOMapIdx));

		CreateMainDepthSRV(device, deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::MainDepthIdx));
		CreateMainDepthSRV(device, ssaoHeap->GetSRVCPUHandle(DescriptorSlot::MainDepthIdx));
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
	CScene* activeScene = GetActiveScene();
	if (!activeScene) return;

	activeScene->Render(commandList);
}

void CSceneManager::ChangeScene(SCENE_TYPE type)
{
	std::shared_ptr<CMyPlayer> myPlayer;
	if (active_scene && active_scene->GetMyPlayer())
		myPlayer = active_scene->GetMyPlayer();

	if (active_scene)
		active_scene->Exit();

	active_scene = scenes[(UINT)type].get();

	if (myPlayer && active_scene->GetSceneType() != SCENE_TYPE::TITLE)
		active_scene->SetPlayer(myPlayer);

	if (type == SCENE_TYPE::LOBBY || type == SCENE_TYPE::GAME) {
		CKeyManager::GetInstance().SetMouseMode(true);
	}
	else {
		CKeyManager::GetInstance().SetMouseMode(false);
	}

	if (active_scene)
		active_scene->Enter();
}

void CSceneManager::CreateMainDepthSRV(ID3D12Device* device)
{
	auto deferredLightingHeap = shaders[EShaderName::Deferred]->GetHeapManager();
	auto ssaoHeap = shaders[EShaderName::SSAO]->GetHeapManager();

	CreateMainDepthSRV(device, deferredLightingHeap->GetSRVCPUHandle(DescriptorSlot::MainDepthIdx));
	CreateMainDepthSRV(device, ssaoHeap->GetSRVCPUHandle(DescriptorSlot::MainDepthIdx));
}

void CSceneManager::CreateMainDepthSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle)
{
	auto depthResource = gGameFramework.GetDepthStencilBuffer().Get();
	if (!depthResource) return;
	auto desc = depthResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	if (depthResource->GetDesc().SampleDesc.Count > 1) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	device->CreateShaderResourceView(depthResource, &srvDesc, srvCpuHandle);
}