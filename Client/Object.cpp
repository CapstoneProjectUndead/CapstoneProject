#include "stdafx.h"
#include "Object.h"

#include "Camera.h"
#include "UIComponent.h"

#include "MeshRenderer.h"
#include "Renderers.h"

CObject::CObject(OBJECT_TYPE type)
	: obj_type(type)
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

void CObject::Initialize()
{
	Update(0.0f);
}

void CObject::ReleaseUploadBuffer()
{
	// 정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	for (auto& component : components)
		if (component->is_enable)
			component->ReleaseUploadBuffer();
}

void CObject::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	for (auto& component : components) {
		if (component->is_enable)
			component->UpdateShaderVariables(commandList);
	}
}

void CObject::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (auto& component : components) {
		component->CreateConstantBuffers(device, commandList);
	}
}

void CObject::Render(ID3D12GraphicsCommandList* commandList)
{
	auto meshRenderer = GetComponents<CMeshRendererComponent>();

	// mesh, collider(for debugging), material render
	for (auto& renderer : meshRenderer)
		if (renderer->is_enable)
			renderer->Render(commandList);
}

void CObject::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
	bool isStatic = (obj_type == OBJECT_TYPE::STATIC_OBJECT);

	for (const auto& meshRenderer : GetComponents<CMeshRendererComponent>()) {
		if (renderers[meshRenderer->GetShader()]) {
			// 메시 렌더러 컴포넌트 내부에서 renderer->AddInstance 호출
			meshRenderer->Collect(renderers[meshRenderer->GetShader()], isStatic);
			// shadow Collect
			meshRenderer->Collect(renderers[EShaderName::Shadow], isStatic);
		}
	}
}

void CObject::SetComponent(std::shared_ptr<CComponent> component)
{
	component->owner = this;
	components.push_back(component);
	component->Initialize();
}

void CObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	world_matrix = Matrix4x4::Multiply(rotateMatrix, world_matrix);
}

void CObject::SetYaw(float _yaw)
{
	yaw = _yaw;
	UpdateLookRightFromYaw();
}

void CObject::SetYawPitch(float yawDeg, float pitchDeg)
{
	// pitch 제한 (이거 중요)
	pitchDeg = std::clamp(pitchDeg, -89.9f, 89.9f);

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(pitchDeg),
		XMConvertToRadians(yawDeg),
		0.0f  
	);

	XMStoreFloat4(&orientation, q);
}

void CObject::UpdateWorldMatrix()
{
	XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&orientation));
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&world_matrix, rot * trans);
}

void CObject::UpdateLookRightFromYaw()
{
	float rad = XMConvertToRadians(yaw);

	look.x = sinf(rad);
	look.y = 0.0f;
	look.z = cosf(rad);

	look = Vector3::Normalize(look);

	// Y-up 기준 Right 벡터
	right = XMFLOAT3(
		look.z,
		0.0f,
		-look.x
	);
}

void CObject::Update(const float elapsedTime)
{
	for (auto& component : components) {
		if (component->is_enable)
			component->Update(elapsedTime);
	}

	local_sphere.Transform(world_sphere, XMLoadFloat4x4(&world_matrix));
}

void CObject::Animate(float elapsedTime, CCamera* camera)
{
	static float angle = 0.0f;
	angle += elapsedTime * 0.5f; // 천천히 회전
	
	XMMATRIX rotY = XMMatrixRotationY(angle);
	XMMATRIX trans = XMMatrixTranslation(world_matrix._41, world_matrix._42, world_matrix._43);

	XMMATRIX world = rotY * trans;
	XMStoreFloat4x4(&world_matrix, world);
}

bool CObject::IsVisible(const BoundingFrustum& frustum)
{
	// DirectX의 내장 함수 사용 (가장 효율적)
	return frustum.Intersects(world_sphere);
}

// CParticleObject
std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
std::uniform_real_distribution<float> speedModifier(0.8f, 1.2f);

CParticleObject::CParticleObject()
	: CObject(OBJECT_TYPE::PARTICLE_OBJECT)
{
	XMMATRIX scaleMatrix = XMMatrixScaling(size.x, size.y, size.z);
	XMStoreFloat4x4(&world_matrix, scaleMatrix);

	InitializeParticles();
}

CParticleObject::CParticleObject(const XMFLOAT3& position)
	: CObject(OBJECT_TYPE::PARTICLE_OBJECT)
{
	XMMATRIX scaleMatrix = XMMatrixScaling(size.x, size.y, size.z);
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&world_matrix, scaleMatrix * translationMatrix);

	InitializeParticles();
}

CParticleObject::CParticleObject(const XMFLOAT3& position, const XMFLOAT4& color)
	: CObject(OBJECT_TYPE::PARTICLE_OBJECT), color(color)
{
	XMMATRIX scaleMatrix = XMMatrixScaling(size.x, size.y, size.z);
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&world_matrix, scaleMatrix * translationMatrix);

	InitializeParticles();
}

void CParticleObject::InitializeParticles()
{
	particles.clear();
	particles.resize(particleCount);

	// 생성 반경 범위를 결정
	std::uniform_real_distribution<float> posOffsetDist(-0.1f, 0.1f);

	for (int i = 0; i < particleCount; ++i) {
		particles[i].material = MaterialData{};
		particles[i].material.albedo = color;
		particles[i].material.emissive_color = color;

		particles[i].spawn_time = 0.0f;
		std::uniform_real_distribution<float> lifeDist(0.8f, 1.5f);
		particles[i].life_time = lifeDist(gen);

		// 불길 속도 세팅
		float randX = dis(gen);
		std::uniform_real_distribution<float> upSpeed(3.0f, 7.0f);
		float randY = upSpeed(gen);
		float randZ = dis(gen);
		particles[i].velocity = XMFLOAT3(randX, randY, randZ);

		XMFLOAT3 objPos = GetPosition();

		// 파티클의 최종 월드 위치 = (오브젝트 기본 위치 + 랜덤 오프셋)
		float offsetX = posOffsetDist(gen);
		float offsetY = std::abs(posOffsetDist(gen)) * 0.5f;
		float offsetZ = posOffsetDist(gen);

		XMMATRIX finalTranslation = XMMatrixTranslation(
			objPos.x + offsetX,
			objPos.y + offsetY,
			objPos.z + offsetZ
		);

		XMMATRIX scaleMatrix = XMMatrixScaling(size.x, size.y, size.z);
		XMStoreFloat4x4(&particles[i].world_matrix, scaleMatrix * finalTranslation);
	}
}

void CParticleObject::Update(const float elapsedTime)
{
	CObject::Update(elapsedTime);

	for (int i = 0; i < particleCount; ++i) {
		particles[i].spawn_time += elapsedTime;

		if (particles[i].spawn_time > particles[i].life_time) {
			particles[i].spawn_time = 0.0f;
		}
	}
}

void CParticleObject::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
	for (int i = 0; i < particleCount; ++i) {
		if (particles[i].spawn_time > particles[i].life_time)
			continue;
		renderers[EShaderName::Billboard]->AddParticleInstance(nullptr, particles[i], false);
	}
}