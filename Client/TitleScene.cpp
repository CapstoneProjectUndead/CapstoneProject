#include "stdafx.h"
#include "TitleScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "Object.inl"

CTitleScene::CTitleScene()
	: CScene(SCENE_TYPE::TITLE)
{
}

CTitleScene::~CTitleScene()
{
}

void CTitleScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	camera = std::make_shared<CCamera>();
	camera->SetTarget(my_player.get());
	camera->Initialize(device, commandList);
}

void CTitleScene::Update(float elapsedTime)
{
}

void CTitleScene::Render(ID3D12GraphicsCommandList*)
{
}

void CTitleScene::Enter()
{
	if (my_player)
		my_player->SetCurrentSceneType(SCENE_TYPE::TITLE);
}

void CTitleScene::Exit()
{
}