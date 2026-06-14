#include "stdafx.h"
#include "Object.h"
#include "Camera.h"

CCamera::CCamera()
	: view_matrix{ Matrix4x4::Identity() },
	projection_matrix{ Matrix4x4::Identity() },
	ortho_matrix{Matrix4x4::Identity()},
	viewport{ 0.0f, 0.0f, float(FRAME_BUFFER_WIDTH), float(FRAME_BUFFER_HEIGHT), 0.0f, 1.0f },
	scissor_rect{ 0, 0, LONG(FRAME_BUFFER_WIDTH), LONG(FRAME_BUFFER_HEIGHT) }
{
}

void CCamera::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	int width{ int(client_rect.right - client_rect.left) };
	int height{ int(client_rect.bottom - client_rect.top) };

	SetViewport(0, 0, width, height);
	SetScissorRect(0, 0, (LONG)width, (LONG)height);
	GenerateProjectionMatrix(0.01f, 20.0f, (float)width / (float)height, 90.0f);
	GenerateOrthoProjectionMatrix(0.0f, 1.0f, (float)width, (float)height);
	SetCameraOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));

	CreateConstantBuffers(device, commandList);
}

void CCamera::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		camera_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<CameraCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		camera_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		ortho_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<OrthoCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		ortho_cb->Map(0, nullptr, reinterpret_cast<void**>(&ortho_mapped));
		billboard_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<BillboardCameraCB>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		billboard_cb->Map(0, nullptr, reinterpret_cast<void**>(&billboard_mapped));
		inv_camera_cb = CreateBufferResource(device, commandList, nullptr, CalculateConstant<XMFLOAT4X4>(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		inv_camera_cb->Map(0, nullptr, reinterpret_cast<void**>(&inv_camera_mapped));
		UINT blurBufferSize = CalculateConstant<CB_BlurInfo>() * 2;	// blur 1 pass 처리 전에 덮어씌움->배열로 처리
		blur_cb = CreateBufferResource(device, commandList, nullptr, blurBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		blur_cb->Map(0, nullptr, reinterpret_cast<void**>(&blur_mapped));
	}
}

void CCamera::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList, bool isUI)
{
	if (isUI) {
		XMStoreFloat4x4(&ortho_mapped->ortho_projection, XMMatrixTranspose(XMLoadFloat4x4(&ortho_matrix)));
		commandList->SetGraphicsRootConstantBufferView(0, ortho_cb->GetGPUVirtualAddress());
		return;
	}

	XMStoreFloat4x4(&mapped->view_matrix, XMMatrixTranspose(XMLoadFloat4x4(&view_matrix)));
	XMStoreFloat4x4(&mapped->projection_matrix, XMMatrixTranspose(XMLoadFloat4x4(&projection_matrix)));

	commandList->SetGraphicsRootConstantBufferView(0, camera_cb->GetGPUVirtualAddress());
}

void CCamera::UpdateShaderVariablesBillBoard(ID3D12GraphicsCommandList* commandList)
{
	XMStoreFloat4x4(&billboard_mapped->view_matrix, XMMatrixTranspose(XMLoadFloat4x4(&view_matrix)));
	XMStoreFloat4x4(&billboard_mapped->projection_matrix, XMMatrixTranspose(XMLoadFloat4x4(&projection_matrix)));
	billboard_mapped->pos = position;

	commandList->SetGraphicsRootConstantBufferView(0, billboard_cb->GetGPUVirtualAddress());
}

void CCamera::UpdateShaderVariablesShadow(ID3D12GraphicsCommandList* commandList)
{
	XMMATRIX view = XMLoadFloat4x4(&view_matrix);
	XMMATRIX proj = XMLoadFloat4x4(&projection_matrix);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMVECTOR det; // 행렬식(Determinant)을 저장할 임시 변수
	XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

	XMStoreFloat4x4(inv_camera_mapped, XMMatrixTranspose(invViewProj));
	commandList->SetGraphicsRootConstantBufferView(3, inv_camera_cb->GetGPUVirtualAddress());
}

void CCamera::UpdateShaderVariablesBlur(ID3D12GraphicsCommandList* commandList, const XMFLOAT2& direction, int clientWidth, int clientHeight, UINT passIndex)
{
	if (!blur_mapped) return;

	UINT alignmentSize = CalculateConstant<CB_BlurInfo>();

	// 오프셋 위치를 계산
	char* bytePtr = reinterpret_cast<char*>(blur_mapped) + (alignmentSize * passIndex);
	CB_BlurInfo* currentBlurInfo = reinterpret_cast<CB_BlurInfo*>(bytePtr);

	// 경계면에 데이터 작성
	currentBlurInfo->blur_direction = direction;
	currentBlurInfo->texel_size = XMFLOAT2(1.0f / clientWidth, 1.0f / clientHeight);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = blur_cb->GetGPUVirtualAddress() + (alignmentSize * passIndex);
	commandList->SetGraphicsRootConstantBufferView(3, cbAddress);
}

void CCamera::GenerateProjectionMatrix(float nearPlaneDistance, float farPlaneDistance, float aspectRatio, float fovAngle)
{
	projection_matrix = Matrix4x4::PerspectiveFovLH(fovAngle, aspectRatio, nearPlaneDistance, farPlaneDistance);
}

void CCamera::GenerateOrthoProjectionMatrix(float nearZ, float farZ, float width, float height)
{
	// farZ를 1000 정도로 크게 잡으면, 10.0이었던 Z값이 0.01 정도로 압축되어 들어옵니다.
	ortho_matrix = Matrix4x4::OrthGraphic(0.0f, width, height, 0.0f, 0.0f, 100.0f);
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
	if (!target_object) return;
	offset = cameraOffset;
	XMVECTOR vOffset = XMLoadFloat3(&offset);
	XMVECTOR vPlayerPos = XMLoadFloat3(&target_object->position); // 플레이어 위치 기준

	XMFLOAT3 xmf3CameraPosition;
	XMStoreFloat3(&xmf3CameraPosition, XMVectorAdd(vPlayerPos, vOffset));

	if (fabsf(offset.z) < 0.0001f) {
		// 1인칭: 플레이어가 바라보는 방향(target_object->look)을 그대로 사용
		SetLookTo(xmf3CameraPosition, target_object->look, target_object->up);
		mode = EMode::FIRST_PERSON;
	}
	else {
		// 3인칭: 플레이어 위치를 바라봄
		SetLookAt(xmf3CameraPosition, target_object->position, target_object->up);
		mode = EMode::THIRD_PERSON;
	}
}

void CCamera::GenerateViewMatrix()
{
	if (mode == EMode::FIRST_PERSON) {
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

void CCamera::SetOrbitMode(bool enable, float initYaw, float initPitch)
{
	// 처음 활성화될 때만 초기 각도 적용 (이미 켜져있다면 현재 각도 유지)
	if (enable && !orbit_mode) {
		orbit_yaw = initYaw;
		orbit_pitch = initPitch;
	}
	orbit_mode = enable;
}

void CCamera::AddOrbitDelta(float yawDelta, float pitchDelta)
{
	orbit_yaw += yawDelta;
	orbit_pitch += pitchDelta;
	orbit_pitch = std::clamp(orbit_pitch, -80.0f, 80.0f);
}

void CCamera::Update(XMFLOAT3& lookAt, float elapsedTime)
{
	if (!target_object) 
		return;

	// 캐릭터의 머리 위치를 기준
	XMVECTOR localEye = target_object->GetHeadPosition();
	XMMATRIX world = XMLoadFloat4x4(&target_object->world_matrix);
	XMVECTOR worldHeadPos = XMVector3TransformCoord(localEye, world);

	if (orbit_mode) {
		// 빈사: 플레이어 회전과 무관한 궤도 카메라.
		// orbit_yaw(좌우)/orbit_pitch(상하)로 offset 벡터를 회전 → 머리 주변 sphere 위 카메라 위치.
		XMMATRIX rot = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(orbit_pitch),
			XMConvertToRadians(orbit_yaw),
			0.0f);
		XMVECTOR offsetWorld = XMVector3TransformCoord(XMLoadFloat3(&offset), rot);
		XMStoreFloat3(&position, worldHeadPos + offsetWorld);

		// 항상 플레이어 머리를 바라봄. up은 월드 기준(누운 자세에도 안정).
		XMFLOAT3 lookTarget;
		XMStoreFloat3(&lookTarget, worldHeadPos);
		XMFLOAT3 worldUp{ 0.0f, 1.0f, 0.0f };
		SetLookAt(position, lookTarget, worldUp);

		GenerateViewMatrix();
		UpdateFrustum();
		return;
	}

	// target 행렬 적용
	XMMATRIX baseRotate;
	baseRotate.r[0] = XMLoadFloat3(&target_object->right);
	baseRotate.r[1] = XMLoadFloat3(&target_object->up);
	baseRotate.r[2] = XMLoadFloat3(&target_object->look);
	baseRotate.r[3] = XMVectorSet(0, 0, 0, 1);

	// 마우스 Pitch 회전 적용
	float pitch = XMConvertToRadians(target_object->GetPitch());
	XMMATRIX pitchRotate = XMMatrixRotationAxis(XMLoadFloat3(&target_object->right), pitch);
	XMMATRIX finalRotate = baseRotate * pitchRotate;

	// 1인칭 카메라 위치 변경
	XMVECTOR vUp = finalRotate.r[1];
	XMVECTOR vLook = finalRotate.r[2];
	XMVECTOR eyePos = worldHeadPos + (vUp * 0.1f) + (vLook * 0.05f);

	switch (mode) {
	case CCamera::EMode::FIRST_PERSON:
		// 위치를 head로 고정
		XMStoreFloat3(&position, eyePos);

		// 시선 방향 업데이트
		XMStoreFloat3(&look, finalRotate.r[2]);
		XMStoreFloat3(&up, finalRotate.r[1]);
		XMStoreFloat3(&right, finalRotate.r[0]);
		break;
	case CCamera::EMode::THIRD_PERSON:
		// 머리 위치를 기준으로 오프셋만큼 뒤로 보냄
		XMVECTOR xmvOffset = XMVector3TransformCoord(XMLoadFloat3(&offset), finalRotate);
		XMStoreFloat3(&position, worldHeadPos + xmvOffset);

		// 시선 처리 (눈 위치를 바라봄)
		XMFLOAT3 p;
		XMStoreFloat3(&p, worldHeadPos);
		SetLookAt(position, p, target_object->up);
		break;
	case CCamera::EMode::FIXED:
		break;
	default:
		break;
	}

	GenerateViewMatrix();

	UpdateFrustum();
}

void CCamera::UpdateFrustum()
{
	// 투영 행렬로부터 절두체 생성
	XMMATRIX proj = XMLoadFloat4x4(&projection_matrix);
	BoundingFrustum::CreateFromMatrix(default_frustum, proj);

	// 뷰 행렬의 역행렬(World Space 변환)을 사용하여 월드 좌표계 절두체로 변환
	XMMATRIX viewInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&view_matrix));
	default_frustum.Transform(frustum, viewInv);
}

void CCamera::PullInToDistance(const XMFLOAT3& lookTarget, float maxDist)
{
	XMVECTOR vTarget = XMLoadFloat3(&lookTarget);
	XMVECTOR vDir = XMVectorSubtract(XMLoadFloat3(&position), vTarget);

	// 타깃과 거의 같은 위치면 방향이 불안정하므로 보정하지 않음
	if (XMVectorGetX(XMVector3Length(vDir)) < 0.0001f)
		return;

	vDir = XMVector3Normalize(vDir);
	XMVECTOR newPos = XMVectorAdd(vTarget, XMVectorScale(vDir, maxDist));
	XMStoreFloat3(&position, newPos);

	// 당겨온 위치에서 다시 타깃을 바라보도록 뷰/절두체 갱신
	SetLookAt(position, lookTarget, up);
	GenerateViewMatrix();
	UpdateFrustum();
}