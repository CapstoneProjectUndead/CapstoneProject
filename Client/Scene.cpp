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
	for (const auto& shader : shaders) {
		shader->ReleaseUploadBuffer();
	}
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ID3D12RootSignature* graphicsRootSignature{};

	// root parameter
	// gameObjectInfo
	D3D12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0; // b0
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// CameraInfo
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 2: MaterialInfo (재질 색 등)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].Descriptor.ShaderRegister = 2; // b2
	rootParameters[2].Descriptor.RegisterSpace = 0;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = rootSignatureFlags;

	// 임의 길이 데이터를 반환하는 데 사용
	ComPtr<ID3DBlob> signatureBlob{};
	ComPtr<ID3DBlob> errorBlob{};
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSignature);

	return graphicsRootSignature;
}

void CScene::AnimateObjects(float elapsedTime)
{
	if (my_player) {
		my_player->Update(elapsedTime);
	}

	for (const auto& obj : objects) {
		obj->Update(elapsedTime);
	}

	for (const auto& shader : shaders) {
		shader->Animate(elapsedTime, camera);
	}
}

void CScene::Update(float elapsedTime)
{
	AnimateObjects(elapsedTime);
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	// set rootsignature
	commandList->SetGraphicsRootSignature(graphics_root_signature.Get());

	// camera set
	if (camera) {
		camera->SetViewportsAndScissorRects(commandList);
		camera->UpdateShaderVariables(commandList);
	}

	for (const auto& shader : shaders) {
		shader->Render(commandList);
	}

	if (my_player) {
		my_player->UpdateShaderVariables(commandList);
		my_player->Render(commandList);
	}

	for (const auto& obj : objects) {
		obj->UpdateShaderVariables(commandList);
		obj->Render(commandList);
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