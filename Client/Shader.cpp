#include "stdafx.h"
#include "Shader.h"
#include "Object.h"
#include "GeometryLoader.h"

void CDescriptorHeapManager::Initialize(ID3D12Device* device, UINT numDescriptors)
{
	num_desc = numDescriptors;
	descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap));

	cpu_start = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	gpu_start = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
}

void CShader::CreateDescriptorHeap(ID3D12Device* device, UINT numDescriptors)
{
	heap_manager = std::make_unique<CDescriptorHeapManager>();
	heap_manager->Initialize(device, numDescriptors);
}

D3D12_INPUT_LAYOUT_DESC CShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 3;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	UINT offset = 0;
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT2);

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
	D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, shaderName, shaderProfile, compileFlags, 0, shaderBlob, error.GetAddressOf());


	if (error) {
		OutputDebugStringA("Shader Compile Error:\n");
		OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
	}

	D3D12_SHADER_BYTECODE shaderBytecode{};
	shaderBytecode.BytecodeLength = (*shaderBlob)->GetBufferSize();
	shaderBytecode.pShaderBytecode = (*shaderBlob)->GetBufferPointer();

	return shaderBytecode;
}

void CShader::CreateShader(ID3D12Device* device)
{
	//그래픽스 파이프라인 상태 객체 배열을 생성한다.
	ComPtr<ID3DBlob> shaderblo{}, pixelShaderBlob{}, gsShaderBlob;
	graphics_root_signature = CreateGraphicsRootSignature(device);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = graphics_root_signature.Get();
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

	device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pipeline_states.GetAddressOf());

	if (pipelineStateDesc.InputLayout.pInputElementDescs) delete[] pipelineStateDesc.InputLayout.pInputElementDescs;
}

ID3D12RootSignature* CShader::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ID3D12RootSignature* graphicsRootSignature{};

	const int numDesc{ 50 };
	D3D12_DESCRIPTOR_RANGE descriptorRanges{};
	// texture
	descriptorRanges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges.NumDescriptors = numDesc;
	descriptorRanges.BaseShaderRegister = 0;
	descriptorRanges.RegisterSpace = 0;
	descriptorRanges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// root parameter
	D3D12_ROOT_PARAMETER rootParameters[4];
	// CameraInfo
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// LightInfo
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// table(texture map)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// instData
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[3].Descriptor.ShaderRegister = 0;
	rootParameters[3].Descriptor.RegisterSpace = 1;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// static sampler
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = rootSignatureFlags;

	// 임의 길이 데이터를 반환하는 데 사용
	ComPtr<ID3DBlob> signatureBlob{};
	ComPtr<ID3DBlob> errorBlob{};
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSignature);

	// CreateHeap
	CreateDescriptorHeap(device, numDesc);

	return graphicsRootSignature;
}

void CShader::RenderBegin(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootSignature(graphics_root_signature.Get());
	commandList->SetPipelineState(pipeline_states.Get());
	if (heap_manager) {
		ID3D12DescriptorHeap* heaps[] = { heap_manager->GetHeap() };
		commandList->SetDescriptorHeaps(1, heaps);
		// 첫 번째 핸들값 Get
		D3D12_GPU_DESCRIPTOR_HANDLE handle = heap_manager->GetGPUHandle(0);
		commandList->SetGraphicsRootDescriptorTable(2, handle);
	}
}

void CShader::Render(ID3D12GraphicsCommandList* commandList, CObject* object)
{
	object->UpdateShaderVariables(commandList);
	object->Render(commandList);
}

// CSkinningShader
D3D12_INPUT_LAYOUT_DESC CSkinningShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 5;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	UINT offset = 0;
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT2);
	inputElementDescs[3] = { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT4);
	inputElementDescs[4] = { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT4);

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE CSkinningShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"SkinningShader.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CSkinningShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"SkinningShader.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

ID3D12RootSignature* CSkinningShader::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ID3D12RootSignature* graphicsRootSignature{};

	const int numDesc{ 50 };
	D3D12_DESCRIPTOR_RANGE descriptorRanges{};
	// texture
	descriptorRanges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges.NumDescriptors = numDesc;
	descriptorRanges.BaseShaderRegister = 0;
	descriptorRanges.RegisterSpace = 0;
	descriptorRanges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// root parameter
	D3D12_ROOT_PARAMETER rootParameters[5];
	// CameraInfo
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// LightInfo
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// table(texture map)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// instData
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[3].Descriptor.ShaderRegister = 0;
	rootParameters[3].Descriptor.RegisterSpace = 1;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// gAnimBuffer
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[4].Descriptor.ShaderRegister = 1;
	rootParameters[4].Descriptor.RegisterSpace = 1;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// static sampler
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = rootSignatureFlags;

	// 임의 길이 데이터를 반환하는 데 사용
	ComPtr<ID3DBlob> signatureBlob{};
	ComPtr<ID3DBlob> errorBlob{};
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSignature);

	// CreateHeap
	CreateDescriptorHeap(device, numDesc);

	return graphicsRootSignature;
}

// CInstShader
D3D12_SHADER_BYTECODE CInstShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"InstShader.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CInstShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"InstShader.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

// CUIShader
D3D12_SHADER_BYTECODE CUIShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"UI.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CUIShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"UI.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

D3D12_RASTERIZER_DESC CUIShader::CreateRasterizerState()
{

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;	// UI에서 삼각형이 뒤집어져서 NONE으로 설정
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}

D3D12_INPUT_LAYOUT_DESC CUIShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 3;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	UINT offset = 0;
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT2);

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

ID3D12RootSignature* CUIShader::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ID3D12RootSignature* graphicsRootSignature{};

	const int numDesc{ 50 };
	D3D12_DESCRIPTOR_RANGE descriptorRanges{};
	// texture
	descriptorRanges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges.NumDescriptors = numDesc;
	descriptorRanges.BaseShaderRegister = 0;
	descriptorRanges.RegisterSpace = 0;
	descriptorRanges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// root parameter
	D3D12_ROOT_PARAMETER rootParameters[3];
	// CameraInfo
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// instData
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].Descriptor.RegisterSpace = 1;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// table(texture map)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// static sampler
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = rootSignatureFlags;

	// 임의 길이 데이터를 반환하는 데 사용
	ComPtr<ID3DBlob> signatureBlob{};
	ComPtr<ID3DBlob> errorBlob{};
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSignature);

	// CreateHeap
	CreateDescriptorHeap(device, numDesc);

	return graphicsRootSignature;
}

// CBillboardShader
D3D12_INPUT_LAYOUT_DESC CBillboardShader::CreateInputLayout()
{
	const UINT inputElementDescNum = 1;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	UINT offset = 0;
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	offset += sizeof(XMFLOAT3);

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE CBillboardShader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"BillboardShader.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CBillboardShader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"BillboardShader.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE CBillboardShader::CreateGeometryShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"BillboardShader.hlsl", "GS", "gs_5_1", shaderBlob);
}

ID3D12RootSignature* CBillboardShader::CreateGraphicsRootSignature(ID3D12Device* device)
{
	ID3D12RootSignature* graphicsRootSignature{};

	const int numDesc{ 5 };
	D3D12_DESCRIPTOR_RANGE descriptorRanges{};
	// texture
	descriptorRanges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges.NumDescriptors = numDesc;
	descriptorRanges.BaseShaderRegister = 0;
	descriptorRanges.RegisterSpace = 0;
	descriptorRanges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// root parameter
	D3D12_ROOT_PARAMETER rootParameters[3];
	// CameraInfo
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// instData
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].Descriptor.RegisterSpace = 1;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// table(texture map)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// static sampler
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = rootSignatureFlags;

	// 임의 길이 데이터를 반환하는 데 사용
	ComPtr<ID3DBlob> signatureBlob{};
	ComPtr<ID3DBlob> errorBlob{};
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSignature);

	// CreateHeap
	CreateDescriptorHeap(device, numDesc);

	return graphicsRootSignature;
}

void CBillboardShader::CreateShader(ID3D12Device* device)
{
	//그래픽스 파이프라인 상태 객체 배열을 생성한다.
	ComPtr<ID3DBlob> shaderblo{}, pixelShaderBlob{}, gsShaderBlob;
	graphics_root_signature = CreateGraphicsRootSignature(device);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = graphics_root_signature.Get();
	pipelineStateDesc.VS = CreateVertexShader(&shaderblo);
	pipelineStateDesc.PS = CreatePixelShader(&pixelShaderBlob);
	pipelineStateDesc.GS = CreateGeometryShader(&gsShaderBlob);
	pipelineStateDesc.RasterizerState = CreateRasterizerState();
	pipelineStateDesc.BlendState = CreateBlendState();
	pipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	pipelineStateDesc.InputLayout = CreateInputLayout();
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pipeline_states.GetAddressOf());

	if (pipelineStateDesc.InputLayout.pInputElementDescs) delete[] pipelineStateDesc.InputLayout.pInputElementDescs;
}


D3D12_RASTERIZER_DESC CBillboardShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}