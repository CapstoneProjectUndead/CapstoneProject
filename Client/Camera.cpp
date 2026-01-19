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

void CCamera::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		CameraCB cb{};
		camera_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<CameraCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	}
}

void CCamera::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	CameraCB cb{};
	XMStoreFloat4x4(&cb.view_matrix, XMMatrixTranspose(XMLoadFloat4x4(&view_matrix)));
	XMStoreFloat4x4(&cb.projection_matrix, XMMatrixTranspose(XMLoadFloat4x4(&projection_matrix)));

	UINT8* mapped = nullptr;
	camera_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &cb, sizeof(cb));
	camera_cb->Unmap(0, nullptr);

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
	XMStoreFloat4x4(&view_matrix, XMMatrixLookAtLH(XMLoadFloat3(&position), XMLoadFloat3(&lookAt), XMLoadFloat3(&ohterUp)));

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
	XMFLOAT3 xmf3CameraPosition;
	XMStoreFloat3(&xmf3CameraPosition, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&offset)));
	SetLookAt(xmf3CameraPosition, position, up);

	GenerateViewMatrix();
}

void CCamera::GenerateViewMatrix()
{
	view_matrix = Matrix4x4::LookAtLH(position, look_at, up);
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
	XMMATRIX rotate;
	rotate.r[0] = XMVectorSet(target_object->right.x, target_object->right.y, target_object->right.z, 0.0f);
	rotate.r[1] = XMVectorSet(target_object->up.x, target_object->up.y, target_object->up.z, 0.0f);
	rotate.r[2] = XMVectorSet(target_object->look.x, target_object->look.y, target_object->look.z, 0.0f);
	rotate.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	XMVECTOR xmvPosition = XMLoadFloat3(&position);
	XMVECTOR xmvOffset = XMVector3TransformCoord(XMLoadFloat3(&offset), rotate);
	XMVECTOR xmvNewPosition = XMVectorAdd(XMLoadFloat3(&target_object->position), xmvOffset);
	XMVECTOR xmvDirection = XMVectorSubtract(xmvNewPosition, xmvPosition);

	float length = XMVectorGetX(XMVector3Length(xmvDirection));
	xmvDirection = XMVector3Normalize(xmvDirection);

	float timeLagScale = elapsedTime * 4.0f;
	float distance = length * timeLagScale;
	if (distance > length) distance = length;
	if (length < 0.01f) distance = length;
	if (distance > 0)
	{
		XMStoreFloat3(&position, XMVectorAdd(xmvPosition, XMVectorScale(xmvDirection, distance)));
		//SetLookAt(target_object->position, target_object->up);
	}

	look_at = lookAt;

	GenerateViewMatrix();
}