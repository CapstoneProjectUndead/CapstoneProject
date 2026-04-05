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
protected:
	EMode mode{ EMode::THIRD_PERSON };

	XMFLOAT4X4 view_matrix;
	XMFLOAT4X4 projection_matrix;
	XMFLOAT4X4 ortho_matrix;
	ComPtr<ID3D12Resource> camera_cb;
	ComPtr<ID3D12Resource> ortho_cb;	// UI에 필요한 값 저장
	ComPtr<ID3D12Resource> billboard_cb;
	CameraCB* mapped{};
	OrthoCB* ortho_mapped{};
	BillboardCameraCB* billboard_mapped{};

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor_rect;

	XMFLOAT3 position{ XMFLOAT3(0.0f, 0.0f, 0.0f) };
	XMFLOAT3 right{ XMFLOAT3(1.0f, 0.0f, 0.0f) };
	XMFLOAT3 up{ XMFLOAT3(0.0f, 1.0f, 0.0f) };
	XMFLOAT3 look{ XMFLOAT3(0.0f, 0.0f, 1.0f) };
	XMFLOAT3 offset{};
	XMFLOAT3 look_at{};

	CObject* target_object{};	// 소유X, 참조용
};