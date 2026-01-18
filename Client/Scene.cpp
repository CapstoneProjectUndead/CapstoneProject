#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Scene.h"
#include "Timer.h"
#include "KeyManager.h"

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
	D3D12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.Num32BitValues = 16;
	rootParameters[0].Constants.ShaderRegister = 0;		// gameObjectInfo
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[1].Constants.Num32BitValues = 32;
	rootParameters[1].Constants.ShaderRegister = 1;		// CameraInfo
	rootParameters[1].Constants.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
	if (player) {
		player->Update(elapsedTime);
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

	if (player) {
		player->UpdateShaderVariables(commandList);
		player->Render(commandList);
	}

	for (const auto& obj : objects) {
		obj->UpdateShaderVariables(commandList);
		obj->Render(commandList);
	}
}
