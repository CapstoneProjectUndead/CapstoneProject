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
	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 otherPosition) { SetPosition(otherPosition.x, otherPosition.y, otherPosition.z); }
	XMFLOAT3 GetPosition() const { return XMFLOAT3(world_matrix._41, world_matrix._42, world_matrix._43); }
	XMFLOAT3 GetLook() const { return(Vector3::Normalize(XMFLOAT3(world_matrix._31, world_matrix._32, world_matrix._33))); }
	XMFLOAT3 GetUp() const { return(Vector3::Normalize(XMFLOAT3(world_matrix._21, world_matrix._22, world_matrix._23)));}
	XMFLOAT3 GetRight() const { return(Vector3::Normalize(XMFLOAT3(world_matrix._11, world_matrix._12, world_matrix._13)));}

protected:
	XMFLOAT4X4 world_matrix;

	std::vector<std::shared_ptr<CMesh>> meshes;
	std::shared_ptr<CTexture> texture{};

	bool is_visible{ true };
	BoundingOrientedBox oobb;
};

class CBillboardObject : public CObject {
public:
	void Animate(float, CCamera*) override;
	void SetLookAt(XMFLOAT3& target);
};
