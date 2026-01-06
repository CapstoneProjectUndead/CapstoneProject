#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Scene.h"
#include "Timer.h"
#include "CKeyMgr.h"

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

void CScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	player = std::make_shared<CPlayer>(device, commandList);
	camera = player->GetCameraPtr();

	// create graphics rootsignature
	graphics_root_signature = CreateGraphicsRootSignature(device);
	
	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(device, graphics_root_signature.Get());
	shader->BuildObjects(device, commandList);
	shaders.push_back(std::move(shader));
}

void CScene::AnimateObjects(float elapsedTime)
{
	player->Update(elapsedTime);
	for (const auto& shader : shaders) {
		shader->Animate(elapsedTime, camera);
	}
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	// set rootsignature
	commandList->SetGraphicsRootSignature(graphics_root_signature.Get());

	// camera set
	camera->SetViewportsAndScissorRects(commandList);
	camera->UpdateShaderVariables(commandList);

	for (const auto& shader : shaders) {
		shader->Render(commandList);
	}

	player->UpdateShaderVariables(commandList);
	player->Render(commandList);
}

void CScene::ProcessInput()
{
	XMFLOAT3 direction{};

	// 창우
	if (KEY_PRESSED(KEY::W)) direction.z++;
	if (KEY_PRESSED(KEY::S)) direction.z--;
	if (KEY_PRESSED(KEY::A)) direction.x--;
	if (KEY_PRESSED(KEY::D)) direction.x++;

	if (direction.x != 0 || direction.z != 0) {
		player->Move(direction, CTimer::GetInstance().GetTimeElapsed());
	}

	CKeyMgr& keyManager{ CKeyMgr::GetInstance() };

	if (KEY_PRESSED(KEY::LBTN) || KEY_PRESSED(KEY::RBTN)) {
		SetCursor(NULL);
		Vec2 prevMousePos{ keyManager.GetPrevMousePos() };
		Vec2 mouseDelta{ (keyManager.GetMousePos() - prevMousePos) / 3.0f };
		if (mouseDelta.x || mouseDelta.y)
		{
			if (KEY_PRESSED(KEY::LBTN))
				player->Rotate(mouseDelta.y, mouseDelta.x, 0.0f);
			if (KEY_PRESSED(KEY::RBTN))
				player->Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}

	}
}
