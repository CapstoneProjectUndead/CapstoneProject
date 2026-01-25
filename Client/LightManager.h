#pragma once

#define MaxLights 16

struct Light
{
	XMFLOAT3 strength;
	float falloff_start;
	XMFLOAT3 direction;
	float falloff_end;
	XMFLOAT3 position;
	float spot_power;
};

struct LightCB
{
	XMFLOAT4 ambientLight;
	XMFLOAT3 eyePosWorld;
	float pad; // 16바이트 정렬 맞추기

	Light lights[MaxLights];
};

class CCamera;

class CLightManager {
public:
	CLightManager() = default;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void Update(const CCamera* camera);
	void Render(ID3D12GraphicsCommandList* commandList);
private:
	LightCB light{};
	ComPtr<ID3D12Resource> light_cb;
};

