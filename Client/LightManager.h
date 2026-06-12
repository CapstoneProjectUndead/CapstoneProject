#pragma once
#ifndef MaxLights
#define MaxLights 20
#endif
#ifndef MAX_POINT_LIGHTS
#define MAX_POINT_LIGHTS MaxLights - 1
#endif

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
	UINT active_dot_num{};

	std::array<std::array<XMFLOAT4X4,6>, MAX_POINT_LIGHTS > cube_shadow_transforms;
	std::array<Light, MaxLights> lights;	// 0: dir, 1~: dot
};

class CCamera;

class CLightManager {
public:
	CLightManager() = default;

	void ClearPointLights();
	bool AddPointLight(const XMFLOAT3& position, const XMFLOAT3& strength, float falloffStart, float falloffEnd);
	bool IsPointLightVisible(UINT dotIdx, const BoundingFrustum& frustum) const
	{
		// 0번은 디렉셔널 라이트이므로, 점 조명은 1번부터 시작합니다.
		UINT arrayIdx = dotIdx + 1;

		BoundingSphere lightSphere;
		lightSphere.Center = light.lights[arrayIdx].position;
		lightSphere.Radius = light.lights[arrayIdx].falloff_end; // 영향 반경

		// 카메라 절두체와 조명 범위가 겹치는지 확인
		return frustum.Intersects(lightSphere);
	}
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	// Frank D. Luna way
	void Update(const CCamera* camera, const BoundingSphere& sceneBounds);
	void Render(ID3D12GraphicsCommandList* commandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	UINT GetActiveDotNum()const { return light.active_dot_num; }
private:
	LightCB light{};
	ComPtr<ID3D12Resource> light_cb;
	LightCB* mapped{};
};

