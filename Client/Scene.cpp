#include "stdafx.h"
#include "Camera.h"
#include "Scene.h"
#include "Timer.h"
#include "GeometryLoader.h"
//#include <iomanip>
#include "KeyManager.h"
#include "Player.h"
#include "MyPlayer.h"

void CScene::ReleaseUploadBuffers()
{
	for (const auto& obj : objects) {
		obj->ReleaseUploadBuffer();
	}
}

void CScene::AnimateObjects(float elapsedTime)
{
	if (my_player) {
		my_player->Update(elapsedTime);
	}

	for (const auto& obj : objects) {
		obj->Update(elapsedTime);
	}
}

void CScene::Update(float elapsedTime)
{
	AnimateObjects(elapsedTime);

	if(camera)
		camera->Update(my_player->position, elapsedTime);
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (int i = 0; i < shaders.size(); ++i) {
		shaders[i]->RenderBegin(commandList);

		if(camera)
			camera->UpdateShaderVariables(commandList);

		for (const auto& obj : objects) {
			if (i == obj->GetShaderIndex())
				shaders[i]->Render(commandList, obj.get());
		}

		if (my_player) {
			if (i == my_player->GetShaderIndex()) {
				my_player->UpdateShaderVariables(commandList);
				my_player->Render(commandList);
			}
		}

		shaders[i]->RenderEnd(commandList);
	}
}

void CScene::EnterScene(std::shared_ptr<CObject> obj, UINT id)
{
	id_To_Index[id] = objects.size();
	objects.push_back(obj);
}

void CScene::LeaveScene(UINT id)
{
	UINT idx = id_To_Index[id];
	UINT last = objects.size() - 1;

	std::swap(objects[idx], objects[last]);
	id_To_Index[objects[idx]->GetID()] = idx;

	objects.pop_back();
	id_To_Index.erase(id);
}