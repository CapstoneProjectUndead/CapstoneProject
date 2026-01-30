#include "stdafx.h"
#include "Camera.h"
#include "Timer.h"
#include "GeometryLoader.h"
#include "KeyManager.h"
#include "Player.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "Scene.h"

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
	if(light)
		light->Update(camera.get());
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (const auto& shader : shaders) {
		shader.second->RenderBegin(commandList);

		if (camera)
			camera->UpdateShaderVariables(commandList);

		if (light)
			light->Render(commandList);

		for (const auto& obj : objects) {
			if (shader.first == obj->GetShader())
				shader.second->Render(commandList, obj.get());
		}

		if (my_player) {
			if (shader.first == my_player->GetShader()) {
				my_player->UpdateShaderVariables(commandList);
				my_player->Render(commandList);
			}
		}

		shader.second->RenderEnd(commandList);
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