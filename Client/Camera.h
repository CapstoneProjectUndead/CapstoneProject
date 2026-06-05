#pragma once

class CObject;

struct CameraCB
{
	XMFLOAT4X4 view_matrix;
	XMFLOAT4X4 projection_matrix;
};

struct BillboardCameraCB
{
	XMFLOAT4X4 view_matrix;
	XMFLOAT4X4 projection_matrix;
	XMFLOAT3 pos;
};

struct OrthoCB
{
	XMFLOAT4X4 ortho_projection;
};


// 생성 시 Initialize, SetTarget 호출
class CCamera
{
public:
	enum class EMode { FIRST_PERSON, THIRD_PERSON, FIXED };
	CCamera();

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList, bool isUI = false);
	virtual void UpdateShaderVariablesBillBoard(ID3D12GraphicsCommandList* commandList);
	virtual void UpdateShaderVariablesShadow(ID3D12GraphicsCommandList* commandList);

	void GenerateProjectionMatrix(float, float, float, float);
	void GenerateOrthoProjectionMatrix(float, float, float, float);
	void SetViewport(int, int, int, int, float = 0.0f, float = 1.0f);
	void SetScissorRect(LONG, LONG, LONG, LONG);
	virtual void SetViewportsAndScissorRects(ID3D12GraphicsCommandList*);

	void SetLookAt(XMFLOAT3 ohterPosition, XMFLOAT3 lookAt, XMFLOAT3 ohterUp);
	void SetLookTo(XMFLOAT3 ohterPosition, XMFLOAT3 lookTo, XMFLOAT3 ohterUp);
	// offset에 따라 카메라 mode가 달라짐
	void SetCameraOffset(XMFLOAT3& cameraOffset);
	void GenerateViewMatrix();
	// normalize 수행 후 vector update
	void UpdateCameraVectors();

	void Rotate(float pitch, float yaw, float roll);
	void Move(const XMFLOAT3 shift);
	void Move(const XMFLOAT3 direction, float distance);
	void Update(XMFLOAT3& lookAt, float elapsedTime);

	XMFLOAT3 GetPos() const { return position; }
	XMFLOAT3 GetOffset() const { return offset; }
	XMFLOAT4X4 GetViewMatrix() const { return view_matrix; }
	XMFLOAT4X4 GetProjectionMatrix() const { return projection_matrix; }
	D3D12_VIEWPORT GetViewPort() const { return viewport; }
	XMFLOAT3 GetPosition() const { return position; }

	void SetTarget(CObject* object) { target_object = object; }
	void SetMode(EMode m) { mode = m; }

	// 빈사 시 사용: 플레이어 회전과 독립된 궤도(Orbit) 카메라.
	// 활성화되면 카메라가 자체 orbit_yaw/orbit_pitch로 플레이어 주변을 돌며 머리를 바라봄.
	void SetOrbitMode(bool enable, float initYaw = 0.0f, float initPitch = 0.0f);
	void AddOrbitDelta(float yawDelta, float pitchDelta);
	bool IsOrbitMode() const { return orbit_mode; }

	void UpdateFrustum();
	BoundingFrustum GetFrustum() const { return frustum; }
protected:
	EMode mode{ EMode::THIRD_PERSON };
	bool  orbit_mode{ false };
	float orbit_yaw{ 0.0f };
	float orbit_pitch{ 0.0f };

	XMFLOAT4X4 view_matrix;
	XMFLOAT4X4 projection_matrix;
	XMFLOAT4X4 ortho_matrix;
	ComPtr<ID3D12Resource> camera_cb;
	ComPtr<ID3D12Resource> ortho_cb;	// UI에 필요한 값 저장
	ComPtr<ID3D12Resource> billboard_cb;
	ComPtr<ID3D12Resource> inv_camera_cb;	// screen shadow에 필요
	CameraCB* mapped{};
	OrthoCB* ortho_mapped{};
	BillboardCameraCB* billboard_mapped{};
	XMFLOAT4X4* inv_camera_mapped{};

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor_rect;

	XMFLOAT3 position{ XMFLOAT3(0.0f, 0.0f, 0.0f) };
	XMFLOAT3 right{ XMFLOAT3(1.0f, 0.0f, 0.0f) };
	XMFLOAT3 up{ XMFLOAT3(0.0f, 1.0f, 0.0f) };
	XMFLOAT3 look{ XMFLOAT3(0.0f, 0.0f, 1.0f) };
	XMFLOAT3 offset{};
	XMFLOAT3 look_at{};

	CObject* target_object{};	// 소유X, 참조용

	BoundingFrustum default_frustum;
	BoundingFrustum frustum;
};