#include "stdafx.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "Camera.h"
#include "Object.h"
#include "Component.h"

// Material
void CMaterial::SetTexture(const std::shared_ptr<CTexture>& tex)
{
	texture = tex;
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	if (!material_cb) return;
	MaterialCB cb{};
	cb.albedo = albedo;
	cb.fresnel = fresnel;
	cb.glossiness = glossiness;

	UINT8* mapped = nullptr;
	material_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &cb, sizeof(cb));
	material_cb->Unmap(0, nullptr);

	commandList->SetGraphicsRootConstantBufferView(2, material_cb->GetGPUVirtualAddress());
}

void CMaterial::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	MaterialCB cb{};
	material_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<MaterialCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
}

std::shared_ptr<CMaterial> CMaterialManager::GetMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex)
{
	auto it = materials.find(name);
	if (it != materials.end())
		return it->second;

	auto mat = std::make_shared<CMaterial>();
	mat->SetTexture(tex);

	materials.emplace(name, mat);
	return mat;
}

// Object
CObject::CObject()
{
	XMStoreFloat4x4(&world_matrix, XMMatrixIdentity());
}

void CObject::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	ComputeBoundingBox();
	CreateConstantBuffers(device, commandList);
	CreateDebugBoundingBoxMesh(device, commandList);

}

void CObject::ReleaseUploadBuffer()
{
	// 정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	for(const auto& mesh : meshes)
		mesh->ReleaseUploadBuffer();
}

void CObject::SetComponent(std::shared_ptr<CComponent> component)
{
	component->owner = this;
	components.push_back(component);
	component->Initialize();
}

void CObject::SetMesh(std::shared_ptr<CMesh>& otherMesh)
{
	meshes.push_back(otherMesh);
}

void CObject::SetMaterial(CMaterial* otherMaterial)
{
	material.reset(otherMaterial);
}

void CObject::SetMaterial(std::shared_ptr<CMaterial>& m)
{
	material = m;
}

void CObject::Rotate(float pitch, float yaw, float roll)
{
	XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll));
	world_matrix = Matrix4x4::Multiply(rotateMatrix, world_matrix);
}

void CObject::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	{
		ObjectCB cb{};
		XMMATRIX worldT = XMMatrixTranspose(XMLoadFloat4x4(&world_matrix));
		XMStoreFloat4x4(&cb.world_matrix, worldT);

		UINT8* mapped = nullptr;
		object_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		memcpy(mapped, &cb, sizeof(cb));
		object_cb->Unmap(0, nullptr);

		commandList->SetGraphicsRootConstantBufferView(0, object_cb->GetGPUVirtualAddress());
	}

	if(material) {
		material->UpdateShaderVariables(commandList);
	}

	for (auto& component : components) {
		component->UpdateShaderVariables(commandList);
	}
}

void CObject::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		ObjectCB cb{};
		object_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<ObjectCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	}

	if (material)
		material->CreateConstantBuffers(device, commandList);

	for (auto& component : components) {
		component->CreateConstantBuffers(device, commandList);
	}
}

void CObject::Render(ID3D12GraphicsCommandList* commandList)
{
	for (const auto& mesh : meshes)
		mesh->Render(commandList);
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
		component->Update(elapsedTime);
	}
}

UINT CObject::GetSRVIndex() const
{
	return material->texture->GetDescriptorIndex();
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

void CObject::ComputeBoundingBox()
{
	XMFLOAT3 minPt(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (auto& mesh : meshes)
	{
		XMFLOAT3 c = mesh->box.Center;
		XMFLOAT3 e = mesh->box.Extents;

		XMFLOAT3 meshMin(c.x - e.x, c.y - e.y, c.z - e.z);
		XMFLOAT3 meshMax(c.x + e.x, c.y + e.y, c.z + e.z);

		minPt.x = min(minPt.x, meshMin.x);
		minPt.y = min(minPt.y, meshMin.y);
		minPt.z = min(minPt.z, meshMin.z);

		maxPt.x = max(maxPt.x, meshMax.x);
		maxPt.y = max(maxPt.y, meshMax.y);
		maxPt.z = max(maxPt.z, meshMax.z);
	}

	// min/max → center/extents 변환
	box.Center = XMFLOAT3(
		(minPt.x + maxPt.x) * 0.5f,
		(minPt.y + maxPt.y) * 0.5f,
		(minPt.z + maxPt.z) * 0.5f
	);

	box.Extents = XMFLOAT3(
		(maxPt.x - minPt.x) * 0.5f,
		(maxPt.y - minPt.y) * 0.5f,
		(maxPt.z - minPt.z) * 0.5f
	);
}

bool CObject::IsColliding(CObject* other)
{
	return box.Intersects(other->box);
}

void CObject::CreateDebugBoundingBoxMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 1) 꼭짓점 계산 (로컬 공간 기준)
	XMFLOAT3 corners[8];
	GetBoxCorners(box, corners);

	struct DebugVertex {
		XMFLOAT3 pos;
		XMFLOAT3 color;
	};

	std::vector<DebugVertex> vertices(8);
	for (int i = 0; i < 8; i++)
	{
		vertices[i].pos = corners[i];
		vertices[i].color = XMFLOAT3(1, 0, 0); // 빨간색
	}

	// 2) CMesh 생성
	debug_bbox_mesh = std::make_shared<CMesh>(device, commandList);

	// 3) 정점 버퍼 생성
	debug_bbox_mesh->SetVertices(device, commandList, 8, vertices);

	// 4) 인덱스 버퍼 생성
	std::vector<UINT> indices(g_BoxLineIndices, g_BoxLineIndices + 24);
	debug_bbox_mesh->SetIndices(device, commandList, 24, indices);

	// 5) 라인 리스트로 설정
	debug_bbox_mesh->primitive_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
}

void CObject::RenderDebugBoundingBox(ID3D12GraphicsCommandList* commandList)
{
	if (!debug_bbox_mesh) return;

	// world_matrix는 이미 ObjectCB로 셰이더에 전달됨
	// 디버그 박스도 동일 world_matrix 사용

	debug_bbox_mesh->Render(commandList);
}