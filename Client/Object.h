#pragma once

class CComponent;
class CShader;
class CCamera;
class CMesh;

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
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

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

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	virtual void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Render(ID3D12GraphicsCommandList* );

	//
	virtual XMFLOAT3 GetHeadPosition() const { return position; };
	void SetPosition(float x, float y, float z) { position = XMFLOAT3(x, y, z); }
	void SetPosition(XMFLOAT3 otherPosition) { SetPosition(otherPosition.x, otherPosition.y, otherPosition.z); }
	XMFLOAT3 GetPosition() const { return position; }
	void SetShdaer(const std::string& name) { shader_name = name; }
	std::string GetShader() const { return shader_name; }

	XMFLOAT3 GetVelocity() { return velocity; }
	void     SetVelocity(const XMFLOAT3& vel) { velocity = vel; }
	void     SetVelocity(float vx, float vy, float vz) { velocity = { vx, vy, vz }; }

	uint64  GetID() const { return obj_id; }
	void	SetID(const uint64 id) { obj_id = id; }

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
	SCENE_TYPE  current_scene_type = SCENE_TYPE::NONE; // 현재 오브젝트가 속한 씬

	std::string shader_name{"static"};	// 적용 쉐이더 이름

	XMFLOAT3 velocity{};
	std::vector<std::shared_ptr<CComponent>> components;

	float jump_power{ 5.0f };
	bool is_grounded{};
	float friction{ 9.0f };

	// 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
	XMFLOAT4	orientation = { 0.f, 0.f, 0.f, 1.f };
	float		yaw = 0.f;
	float		pitch = 0.f;
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