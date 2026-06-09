#pragma once

class CComponent;
class CShader;
class CCamera;
class CMesh;
class IRenderer;

namespace CGeometryLoader {
	struct FrameNode;
	struct Mesh;
}

struct ObjectCB
{
	XMFLOAT4X4 world_matrix;
};

// mesh를 가지고 있는 게임 오브젝트 클래스
class CObject{
public:
	CObject(OBJECT_TYPE type);

	void ReleaseUploadBuffer();
	// 항상 값 초기화 후 마지막에 호출
	virtual void Initialize();

	//set
	void SetComponent(std::shared_ptr<CComponent> component);
	//get
	template<typename T>
	T* GetComponent() const;
	template<typename T>
	std::vector<T*> GetComponents() const;

	virtual void Animate(float, CCamera*);
	virtual void Update(const float);
	virtual void Rotate(float pitch, float yaw, float roll);

	// 절두체 컬링 확인
	bool IsVisible(const BoundingFrustum& frustum);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	virtual void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Render(ID3D12GraphicsCommandList* );
	// RenderCallback을 호출
	virtual void OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers);
	
	void SetBoundingSphere(const XMFLOAT3& center, float radius) { local_sphere.Center = center; local_sphere.Radius = radius; };
	void SetBoundingSphere(const BoundingSphere& sphere) { local_sphere = sphere; };
	BoundingSphere GetBoundingSphere() const { return local_sphere; };

	// 로컬 메시 AABB. 정적 오브젝트는 매 프레임 Update를 돌지 않아 world AABB가 없으므로,
	// 카메라 충돌 검사 등에서 이 로컬 박스를 world_matrix로 그때그때 변환해 사용한다.
	void SetLocalAABB(const BoundingBox& box) { local_aabb = box; };
	BoundingBox GetLocalAABB() const { return local_aabb; };

	//
	virtual XMVECTOR GetHeadPosition() const { return XMLoadFloat3(&position); };
	void SetPosition(float x, float y, float z) { position = XMFLOAT3(x, y, z); }
	void SetPosition(XMFLOAT3 otherPosition) { SetPosition(otherPosition.x, otherPosition.y, otherPosition.z); }
	XMFLOAT3 GetPosition() const { return position; }

	void SetIsGrounded(bool ground) { is_grounded = ground; }

	XMFLOAT3 GetVelocity() { return velocity; }
	void     SetVelocity(const XMFLOAT3& vel) { velocity = vel; }
	void     SetVelocity(float vx, float vy, float vz) { velocity = { vx, vy, vz }; }

	uint64  GetID() const { return obj_id; }
	void	SetID(const uint64 id) { obj_id = id; }

	bool IsPendingDelete() const { return pending_delete; }
	void MarkForDelete()         { pending_delete = true; }

	OBJECT_TYPE GetObjectType() const { return obj_type; }
	void        SetObjectType(OBJECT_TYPE type) { obj_type = type; }

	SCENE_TYPE GetCurrentSceneType() const { return current_scene_type; }
	void       SetCurrentSceneType(const SCENE_TYPE type) { current_scene_type = type; }

	//=================================
	// 회전 함수 (테스트)
	void SetYaw(float _yaw);
	float GetYaw() const { return yaw; }
	void SetPitch(float _pitch) { pitch = _pitch; }
	float GetPitch() const { return pitch; }
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

	// component
	friend class CMovementComponent;
	friend class CAnimatorComponent;
	friend class CPhysicsManager;
	std::string name;	// 디버깅용
protected:
	uint64      obj_id = -1;	// 모든 오브젝트는 고유 식별 ID를 가진다.
	OBJECT_TYPE obj_type;
	bool        pending_delete = false;
	SCENE_TYPE  current_scene_type = SCENE_TYPE::NONE; // 현재 오브젝트가 속한 씬

	XMFLOAT3 velocity{};
	std::vector<std::shared_ptr<CComponent>> components;

	float jump_power{ 4.0f };
	bool is_grounded{true};
	float friction{ 9.0f };

	// 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
	XMFLOAT4	orientation = { 0.f, 0.f, 0.f, 1.f };
	float		yaw = 0.f;
	float		pitch = 0.f;

	// 절두체 컬링을 위한 Bounding
	BoundingSphere local_sphere;
	BoundingSphere world_sphere;
	// 카메라 충돌용 로컬 메시 AABB (extents 0 = 미설정)
	BoundingBox local_aabb{ XMFLOAT3{0.0f, 0.0f, 0.0f}, XMFLOAT3{0.0f, 0.0f, 0.0f} };
};

template<typename T>
T* CObject::GetComponent() const
{
	for (auto& comp : components)
	{
		if (T* casted = dynamic_cast<T*>(comp.get()))
			return casted;
	}
	return nullptr;
}

template<typename T>
std::vector<T*> CObject::GetComponents() const
{
	std::vector<T*> result;

	for (auto& comp : components)
	{
		if (T* casted = dynamic_cast<T*>(comp.get()))
			result.push_back(casted);
	}

	return result;
}

class CParticleObject : public CObject {
public:
	CParticleObject();
	CParticleObject(const XMFLOAT4& color);
	void OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers) override;
private:
	XMFLOAT4 color{ 1.0, 0,0,1 };
};