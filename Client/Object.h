#pragma once

class CComponent;
class CShader;
class CCamera;
class CMesh;
class CTexture;

// GeometryLoader에 정의
struct Mesh;
struct FrameNode;

class CMaterial
{
public:
	CMaterial() = default;
	void SetTexture(const std::shared_ptr<CTexture>& tex);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
public:
	XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };	// 프레넬 효과 반사양
	float glossiness{ 0.25f };

	std::shared_ptr<CTexture> texture;
	ComPtr<ID3D12Resource> material_cb;
};

class CMaterialManager
{
public:
	// 없으면 생성
	std::shared_ptr<CMaterial> GetMeterial(const std::string& name, const std::shared_ptr<CTexture>& tex);
private:
	std::unordered_map<std::string, std::shared_ptr<CMaterial>> materials;
};

struct MaterialCB
{
	XMFLOAT4  albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 fresnel{ 0.01f, 0.01f,0.01f };
	float glossiness{ 0.25f };
};

struct ObjectCB
{
	XMFLOAT4X4 world_matrix;
};

// mesh를 가지고 있는 게임 오브젝트 클래스
class CObject{
public:
	CObject();

	void ReleaseUploadBuffer();
	// 항상 값 초기화 후 마지막에 호출
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	//set
	void SetComponent(std::shared_ptr<CComponent> component);
	void SetMesh(std::shared_ptr<CMesh>& otherMesh);
	void SetMaterial(CMaterial* );
	void SetMaterial(std::shared_ptr<CMaterial>&);
	// LoadFrame 정보 Set, T: Vertex type
	template<typename T>
	void SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<FrameNode>& node);
	//get
	template<typename T>
	T* GetComponent();
	UINT GetSRVIndex() const;

	virtual void Animate(float, CCamera*);
	virtual void Update(const float);
	virtual void Rotate(float pitch, float yaw, float roll);

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList);
	virtual void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Render(ID3D12GraphicsCommandList* );

	//
	void SetPosition(float x, float y, float z) { position = XMFLOAT3(x, y, z); }
	void SetPosition(XMFLOAT3 otherPosition) { SetPosition(otherPosition.x, otherPosition.y, otherPosition.z); }
	void SetShdaer(const std::string& name) { shader_name = name; }
	std::string GetShader() const { return shader_name; }

	XMFLOAT3 GetVelocity() { return velocity; }
	void     SetVelocity(const XMFLOAT3& vel) { velocity = vel; }
	void     SetVelocity(float vx, float vy, float vz) { velocity = { vx, vy, vz }; }

	int  GetID() const { return obj_id; }
	void SetID(const int id) { obj_id = id; }
	//=================================
	// 회전 함수 (테스트)
	void SetYaw(float _yaw);
	void SetYawPitch(float yawDeg, float pitchDeg);
	void UpdateWorldMatrix();
	void UpdateLookRightFromYaw();
	//=================================
	// 충돌관련
	// mesh들의 boundingbox 계산
	void ComputeBoundingBox();
	bool IsColliding(CObject* other);
	void CreateDebugBoundingBoxMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void RenderDebugBoundingBox(ID3D12GraphicsCommandList* commandList);
	
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
protected:
	int obj_id = -1;	// 모든 오브젝트는 고유 식별 ID를 가진다.

	std::vector<std::shared_ptr<CMesh>> meshes;
	std::shared_ptr<CMaterial> material;
	std::string shader_name{"static"};	// 적용 쉐이더 이름

	ComPtr<ID3D12Resource> object_cb;

	XMFLOAT3 velocity{};
	std::vector<std::shared_ptr<CComponent>> components;

	bool is_visible{ true };

	BoundingBox box;      // 로컬 공간 기준
	std::shared_ptr<CMesh> debug_bbox_mesh;  // 디버그용 라인 박스
	
	// 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
	XMFLOAT4	orientation = { 0.f, 0.f, 0.f, 1.f };
	float		yaw = 0.f;
	float		pitch = 0.f;
};

template<typename T>
T* CObject::GetComponent()
{
	for (auto& comp : components)
	{
		if (T* casted = dynamic_cast<T*>(comp.get()))
			return casted;
	}
	return nullptr;
}

static const UINT g_BoxLineIndices[24] =
{
	0,1, 1,2, 2,3, 3,0,   // 앞면
	4,5, 5,6, 6,7, 7,4,   // 뒷면
	0,4, 1,5, 2,6, 3,7    // 앞-뒤 연결
};

static void GetBoxCorners(const BoundingBox& box, XMFLOAT3 out[8])
{
	XMFLOAT3 c = box.Center;
	XMFLOAT3 e = box.Extents;

	out[0] = { c.x - e.x, c.y - e.y, c.z - e.z };
	out[1] = { c.x + e.x, c.y - e.y, c.z - e.z };
	out[2] = { c.x + e.x, c.y + e.y, c.z - e.z };
	out[3] = { c.x - e.x, c.y + e.y, c.z - e.z };

	out[4] = { c.x - e.x, c.y - e.y, c.z + e.z };
	out[5] = { c.x + e.x, c.y - e.y, c.z + e.z };
	out[6] = { c.x + e.x, c.y + e.y, c.z + e.z };
	out[7] = { c.x - e.x, c.y + e.y, c.z + e.z };
}