#include "stdafx.h"
#include "Object.h"
#include "Camera.h"

CCamera::CCamera()
	: view_matrix{ Matrix4x4::Identity() },
	projection_matrix{ Matrix4x4::Identity() },
	viewport{ 0.0f, 0.0f, float(FRAME_BUFFER_WIDTH), float(FRAME_BUFFER_HEIGHT), 0.0f, 1.0f },
	scissor_rect{ 0, 0, LONG(FRAME_BUFFER_WIDTH), LONG(FRAME_BUFFER_HEIGHT) }
{
}

void CCamera::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	SetViewport(0, 0, width, height);
	SetScissorRect(0, 0, width, height);
	GenerateProjectionMatrix(0.01f, 500.0f, (float)width / (float)height, 90.0f);
	SetCameraOffset(XMFLOAT3(0.0f, 0.5f, -1.0f));

	CreateConstantBuffers(device, commandList);
}

void CCamera::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		camera_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<CameraCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		camera_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	}
}

void CCamera::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	XMStoreFloat4x4(&mapped->view_matrix, XMMatrixTranspose(XMLoadFloat4x4(&view_matrix)));
	XMStoreFloat4x4(&mapped->projection_matrix, XMMatrixTranspose(XMLoadFloat4x4(&projection_matrix)));

	commandList->SetGraphicsRootConstantBufferView(1, camera_cb->GetGPUVirtualAddress());
}

void CCamera::GenerateProjectionMatrix(float nearPlaneDistance, float farPlaneDistance, float aspectRatio, float fovAngle)
{
	projection_matrix = Matrix4x4::PerspectiveFovLH(fovAngle, aspectRatio, nearPlaneDistance, farPlaneDistance);
}

void CCamera::SetViewport(int x, int y, int width, int height, float minZ, float maxZ)
{
	viewport.TopLeftX = (float)x;
	viewport.TopLeftY = (float)y;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = minZ;
	viewport.MaxDepth = maxZ;
}

void CCamera::SetScissorRect(LONG left, LONG top, LONG right, LONG bottom)
{
	scissor_rect.left = left;
	scissor_rect.top = top;
	scissor_rect.right = right;
	scissor_rect.bottom = bottom;
}

void CCamera::SetViewportsAndScissorRects(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissor_rect);
}

void CCamera::SetLookAt(XMFLOAT3 ohterPosition, XMFLOAT3 lookAt, XMFLOAT3 ohterUp)
{
	position = ohterPosition;
	look_at = lookAt;
	view_matrix = Matrix4x4::LookAtLH(position, lookAt, ohterUp);

	UpdateCameraVectors();
}

void CCamera::SetLookTo(XMFLOAT3 ohterPosition, XMFLOAT3 lookTo, XMFLOAT3 ohterUp)
{
	position = ohterPosition;
	// LookTo는 방향을 받으므로 look_at 지점을 역산해서 저장 (디버깅용)
	look_at = Vector3::Add(position, lookTo);

	view_matrix = Matrix4x4::LookToLH(position, lookTo, ohterUp);
	UpdateCameraVectors();
}

void CCamera::UpdateCameraVectors()
{
	XMVECTORF32 xm32vRight = { view_matrix._11, view_matrix._21, view_matrix._31, 0.0f };
	XMVECTORF32 xm32vUp = { view_matrix._12, view_matrix._22, view_matrix._32, 0.0f };
	XMVECTORF32 xm32vLook = { view_matrix._13, view_matrix._23, view_matrix._33, 0.0f };

	XMStoreFloat3(&right, XMVector3Normalize(xm32vRight));
	XMStoreFloat3(&up, XMVector3Normalize(xm32vUp));
	XMStoreFloat3(&look, XMVector3Normalize(xm32vLook));
}

void CCamera::SetCameraOffset(XMFLOAT3& cameraOffset)
{
	offset = cameraOffset;
	XMVECTOR vOffset = XMLoadFloat3(&offset);
	XMVECTOR vPlayerPos = XMLoadFloat3(&target_object->position); // 플레이어 위치 기준

	XMFLOAT3 xmf3CameraPosition;
	XMStoreFloat3(&xmf3CameraPosition, XMVectorAdd(vPlayerPos, vOffset));

	if (fabsf(offset.z) < 0.0001f) {
		// 1인칭: 플레이어가 바라보는 방향(target_object->look)을 그대로 사용
		SetLookTo(xmf3CameraPosition, target_object->look, target_object->up);
		mode = ECameraMode::FIRST_PERSON;
	}
	else {
		// 3인칭: 플레이어 위치를 바라봄
		SetLookAt(xmf3CameraPosition, target_object->position, target_object->up);
		mode = ECameraMode::THIRD_PERSON;
	}
}

void CCamera::GenerateViewMatrix()
{
	if (mode == ECameraMode::FIRST_PERSON) {
		// 위치, 바라보는 방향(look), 하늘 방향(up)을 이용
		view_matrix = Matrix4x4::LookToLH(position, look, up);
	}
	else {
		// 3인칭 LookAt 사용
		view_matrix = Matrix4x4::LookAtLH(position, look_at, up);
	}
}

void CCamera::Rotate(float pitch, float yaw, float roll)
{
	if (pitch != 0.0f) {
		XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(&right), XMConvertToRadians(pitch));
		XMStoreFloat3(&look, XMVector3TransformNormal(XMLoadFloat3(&look), rotate));
		XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), rotate));
	}
	if (yaw != 0.0f) {
		XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(&up), XMConvertToRadians(yaw));
		XMStoreFloat3(&look, XMVector3TransformNormal(XMLoadFloat3(&look), rotate));
		XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&right), rotate));
	}
	if (roll != 0.0f) {
		XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(&look), XMConvertToRadians(roll));
		XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), rotate));
		XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&right), rotate));
	}
}

void CCamera::Move(const XMFLOAT3 direction, float distance)
{
	XMFLOAT3 shift = XMFLOAT3(0, 0, 0);
	if (direction.z > 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&look), distance)));
	}if (direction.z < 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&look), -distance)));
	}if (direction.x < 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&right), -distance)));
	}if (direction.x > 0) {
		XMStoreFloat3(&shift, XMVectorAdd(XMLoadFloat3(&shift), XMVectorScale(XMLoadFloat3(&right), distance)));
	}
	Move(shift);
}

void CCamera::Move(const XMFLOAT3 shift)
{
	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&shift)));
}

void CCamera::Update(XMFLOAT3& lookAt, float elapsedTime)
{
	float targetPitch = XMConvertToRadians(target_object->GetPitch());
	float targetYaw = XMConvertToRadians(target_object->GetYaw());

	if (mode == ECameraMode::FIRST_PERSON) {
		XMVECTOR newPosition = XMLoadFloat3(&target_object->position) + XMLoadFloat3(&offset);
		XMStoreFloat3(&position, newPosition);

		XMMATRIX bowRotationMatrix = XMMatrixRotationRollPitchYaw(targetPitch, targetYaw, 0.0f);
		XMStoreFloat3(&look, bowRotationMatrix.r[2]);
		XMStoreFloat3(&up, bowRotationMatrix.r[1]);
		XMStoreFloat3(&right, bowRotationMatrix.r[0]);
	}
	else {
		XMMATRIX baseRotate;
		baseRotate.r[0] = XMLoadFloat3(&target_object->right);
		baseRotate.r[1] = XMLoadFloat3(&target_object->up);
		baseRotate.r[2] = XMLoadFloat3(&target_object->look);
		baseRotate.r[3] = XMVectorSet(0, 0, 0, 1);
		XMMATRIX pitchRotate = XMMatrixRotationAxis(XMLoadFloat3(&target_object->right), targetPitch);
		XMMATRIX finalRotate = baseRotate * pitchRotate;

		XMVECTOR xmvPosition = XMLoadFloat3(&position);
		XMVECTOR xmvOffset = XMVector3TransformCoord(XMLoadFloat3(&offset), finalRotate);
		XMVECTOR xmvNewPosition = XMVectorAdd(XMLoadFloat3(&target_object->position), xmvOffset);
		XMVECTOR xmvDirection = XMVectorSubtract(xmvNewPosition, xmvPosition);

		float length = XMVectorGetX(XMVector3Length(xmvDirection));
		xmvDirection = XMVector3Normalize(xmvDirection);

		float timeLagScale = elapsedTime * 4.0f;
		float distance = length * timeLagScale;
		if (distance > length) distance = length;
		if (length < 0.01f) distance = length;
		if (distance > 0) {
			XMStoreFloat3(&position, XMVectorAdd(xmvPosition, XMVectorScale(xmvDirection, distance)));
			//SetLookAt(target_object->position, target_object->up);
		}

		look_at = lookAt;
	}

	GenerateViewMatrix();
}