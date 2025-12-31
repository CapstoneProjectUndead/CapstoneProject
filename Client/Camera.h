#pragma once

class CPlayer;

class CCamera
{
public:
	CCamera();

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList*);

	void GenerateProjectionMatrix(float, float, float, float);
	void SetViewport(int, int, int, int, float = 0.0f, float = 1.0f);
	void SetScissorRect(LONG, LONG, LONG, LONG);
	virtual void SetViewportsAndScissorRects(ID3D12GraphicsCommandList*);

	void SetLookAt(XMFLOAT3 ohterPosition, XMFLOAT3 lookAt, XMFLOAT3 ohterUp);
	void SetCameraOffset(XMFLOAT3& cameraOffset);
	void GenerateViewMatrix();

	void Rotate(float pitch, float yaw, float roll);
	void Move(const XMFLOAT3 shift);
	void Move(const XMFLOAT3 direction, float distance);
	void Update(XMFLOAT3& lookAt, float elapsedTime);

	XMFLOAT3 GetPos() const { return position; }
	XMFLOAT3 GetOffset() const { return offset; }

	void SetPlayer(CPlayer* otherPlayer) { player = otherPlayer; }
protected:
	XMFLOAT4X4 view_matrix;
	XMFLOAT4X4 projection_matrix;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor_rect;

	XMFLOAT3 position{ XMFLOAT3(0.0f, 0.0f, 0.0f) };
	XMFLOAT3 right{ XMFLOAT3(1.0f, 0.0f, 0.0f) };
	XMFLOAT3 up{ XMFLOAT3(0.0f, 1.0f, 0.0f) };
	XMFLOAT3 look{ XMFLOAT3(0.0f, 0.0f, 1.0f) };
	XMFLOAT3 offset{};
	XMFLOAT3 look_at{};

	CPlayer* player;
};


