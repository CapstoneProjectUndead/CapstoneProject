#pragma once
#include "Mesh.h"
#include "Texture.h"

class CShader;
class CCamera;

// mesh를 가지고 있는 게임 오브젝트 클래스
class CObject{
public:
	CObject();

	void ReleaseUploadBuffer();

	void SetMesh(std::shared_ptr<CMesh>& otherMesh);
	void SetTexture(CTexture* );
	CTexture* GetTexture() const { return texture.get(); }
	ID3D12Resource* GetTextureResource() const { return texture->GetTextureResource(); }

	virtual void Animate(float, CCamera*);
	virtual void Update(float) {};
	virtual void Rotate(float pitch, float yaw, float roll);
	virtual void Move(const XMFLOAT3 direction, float distance);
	virtual void Move(const XMFLOAT3 shift);

	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList* );

	//
	void SetPosition(float x, float y, float z) { position = XMFLOAT3(x, y, z); }
	void SetPosition(XMFLOAT3 otherPosition) { SetPosition(otherPosition.x, otherPosition.y, otherPosition.z); }

	int  GetID() const { return obj_id; }
	void SetID(const int id) { obj_id = id; }

	//=================================
	// 회전 함수 (테스트)
	void SetYaw(float _yaw);
	void SetYawPitch(float yawDeg, float pitchDeg);
	void UpdateWorldMatrix();
	void UpdateLookRightFromYaw();
	//=================================

public:
	XMFLOAT4X4 world_matrix;

	// world_matrix 내부 메모리를 직접 참조
	XMFLOAT3& right = *(XMFLOAT3*)&world_matrix._11;
	XMFLOAT3& up = *(XMFLOAT3*)&world_matrix._21;
	XMFLOAT3& look = *(XMFLOAT3*)&world_matrix._31;
	XMFLOAT3& position = *(XMFLOAT3*)&world_matrix._41;

protected:
	int obj_id = -1;	// 모든 오브젝트는 고유 식별 ID를 가진다.

	std::vector<std::shared_ptr<CMesh>> meshes;
	std::shared_ptr<CTexture> texture{};

	bool is_visible{ true };
	BoundingOrientedBox oobb;

	float speed{ 10.0f };

	// 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
	XMFLOAT4	orientation = { 0.f, 0.f, 0.f, 1.f };
	float		yaw = 0.f;
	float		pitch = 0.f;
};