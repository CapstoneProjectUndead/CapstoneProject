#include "stdafx.h"
#include "UIScene.h"

CUIScene::CUIScene()
	:CScene(SCENE_TYPE::UI)
{
}

void CUIScene::BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*)
{
}

void CUIScene::DrawUI()
{
}

bool CUIScene::IsUIInputEnabled()
{
	return false;
}
