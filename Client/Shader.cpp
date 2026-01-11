#include "stdafx.h"
#include "Shader.h"
#include "Object.h"
#include "GeometryLoader.h"

CShader::CShader()
{
}

CShader::~CShader()
{
}

void CShader::ReleaseUploadBuffer()
{
	for (const auto& object : objects) {
		object->ReleaseUploadBuffer();
	}
}

void CShader::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		auto object = std::make_unique<CObject>();
		object->SetPosition(XMFLOAT3(-2.0f, 1.0f, 0.0f));

		auto rootFrame = CGeometryLoader::LoadGeometry("../Modeling/undead_char.bin", device, commandList);
		object->SetMesh(rootFrame->mesh);
		objects.push_back(std::move(object));
	}

	{
		auto object = std::make_unique<CObject>();
		object->SetPosition(XMFLOAT3(2.0f, 0.0f, 0.0f));
		std::shared_ptr<CMesh> mesh = std::make_shared<CCubeMesh>(device, commandList);

		object->SetMesh(mesh);
		objects.push_back(std::move(object));
	}
}

D3D12_INPUT_LAYOUT_DESC CShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 2;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	//정점은 위치 벡터(POSITION)와 색상(COLOR)을 가진다. 
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

D3D12_RASTERIZER_DESC CShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}

D3D12_BLEND_DESC CShader::CreateBlendState()
{
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

D3D12_DEPTH_STENCIL_DESC CShader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}


D3D12_SHADER_BYTECODE CShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"Shaders.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"Shaders.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CShader::CreateGeometryShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"Shaders.hlsl", "PSMain", "gs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CShader::CompileShaderFromFile(WCHAR* fileName, LPCSTR shaderName, LPCSTR shaderProfile, ID3DBlob** shaderBlob)
{
	UINT compileFlags{};
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ComPtr<ID3DBlob> error;
	D3DCompileFromFile(fileName, nullptr, nullptr, shaderName, shaderProfile, compileFlags, 0, shaderBlob, error.GetAddressOf());


	if (error) {
		OutputDebugStringA("Shader Compile Error:\n");
		OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
	}

	D3D12_SHADER_BYTECODE shaderBytecode{};
	shaderBytecode.BytecodeLength = (*shaderBlob)->GetBufferSize();
	shaderBytecode.pShaderBytecode = (*shaderBlob)->GetBufferPointer();

	return shaderBytecode;
}

void CShader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	//그래픽스 파이프라인 상태 객체 배열을 생성한다.
	pipeline_states.push_back(ComPtr<ID3D12PipelineState>());
	ComPtr<ID3DBlob> shaderblo{}, pixelShaderBlob{}, gsShaderBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = rootSignature;
	pipelineStateDesc.VS = CreateVertexShader(&shaderblo);
	pipelineStateDesc.PS = CreatePixelShader(&pixelShaderBlob);
	pipelineStateDesc.RasterizerState = CreateRasterizerState();
	pipelineStateDesc.BlendState = CreateBlendState();
	pipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	pipelineStateDesc.InputLayout = CreateInputLayout();
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pipeline_states[0].GetAddressOf());

	if (pipelineStateDesc.InputLayout.pInputElementDescs) delete[] pipelineStateDesc.InputLayout.pInputElementDescs;
}

void CShader::Animate(float elapsedTime, CCamera* camera)
{
	for (const auto& object : objects) {
		object->Animate(elapsedTime, camera);
	}
}

void CShader::Render(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(pipeline_states[0].Get());

	for (const auto& object : objects) {
		object->UpdateShaderVariables(commandList);
		object->Render(commandList);
	}
}

// CTextureShader
D3D12_INPUT_LAYOUT_DESC CTextureShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 2;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	inputElementDescs[0] = {
		"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};

	inputElementDescs[1] = {
		"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE CTextureShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"TexShader.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CTextureShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"TexShader.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CTextureShader::CreateGeometryShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"TexShader.hlsl", "GS", "gs_5_1", shaderBlob);
}

ID3D12DescriptorHeap* CTextureShader::CreateDescriptorHeap(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc{};
	HeapDesc.NumDescriptors = 1;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ID3D12DescriptorHeap* descriptorHeap{};
	ThrowIfFailed(device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

D3D12_BLEND_DESC CTextureShader::CreateBlendState()
{
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = TRUE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

D3D12_DEPTH_STENCIL_DESC CTextureShader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}

void CTextureShader::CreateShader(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	//그래픽스 파이프라인 상태 객체 배열을 생성한다.
	pipeline_states.push_back(ComPtr<ID3D12PipelineState>());
	ComPtr<ID3DBlob> shaderblo{}, pixelShaderBlob{}, gsShaderBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = rootSignature;
	pipelineStateDesc.VS = CreateVertexShader(&shaderblo);
	pipelineStateDesc.PS = CreatePixelShader(&pixelShaderBlob);
	pipelineStateDesc.GS = CreateGeometryShader(&gsShaderBlob);
	pipelineStateDesc.RasterizerState = CreateRasterizerState();
	pipelineStateDesc.BlendState = CreateBlendState();
	pipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	pipelineStateDesc.InputLayout = CreateInputLayout();
	pipelineStateDesc.SampleMask = UINT_MAX;
	//pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pipeline_states[0].GetAddressOf());

	if (pipelineStateDesc.InputLayout.pInputElementDescs) delete[] pipelineStateDesc.InputLayout.pInputElementDescs;
}


void CTextureShader::CreateShaderVariables(ID3D12Device* device)
{
	descriptor_heap = CreateDescriptorHeap(device);
}

void CTextureShader::Render(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(pipeline_states[0].Get());
}
