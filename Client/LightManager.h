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
	XMFLOAT4X4 shadow_transform{Matrix4x4::Identity()};
	XMFLOAT4X4 shadow_view_proj;     // Shadow Pass용 (World -> NDC)
	XMFLOAT4 ambient_light;
	XMFLOAT3 eyePos_world;
	float pad; // 16바이트 정렬 맞추기

	XMFLOAT4X4 cube_shadow_transforms[6];
	Light lights[MaxLights];
};

class CCamera;

class CLightManager {
public:
	CLightManager() = default;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	// Frank D. Luna way
	void Update(const CCamera* camera, const BoundingSphere& sceneBounds);
	void Render(ID3D12GraphicsCommandList* commandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
private:
	LightCB light{};
	ComPtr<ID3D12Resource> light_cb;
	LightCB* mapped{};
};

