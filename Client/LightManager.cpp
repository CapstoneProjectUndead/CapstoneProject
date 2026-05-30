#include "stdafx.h"
#include "LightManager.h"
#include "Camera.h"

void CLightManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    light_cb = CreateBufferResource(
        device,
        commandList,
        nullptr,
        CalculateConstant<LightCB>(),
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr
    );
    light_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    // light 초기화
    light.ambient_light = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

    // 방향광 3개 예시
    light.lights[0].direction = XMFLOAT3(0.5 , -1.0, 0.01);
    light.lights[0].strength = XMFLOAT3(0.7f, 0.7, 0.7);

    light.lights[1].position = XMFLOAT3(-1.0f, 1.5f, 3.0f); // 조명 위치
    light.lights[1].falloff_end = 5.0f;                   // 조명 최대 반경 (FarZ로 활용)
    light.lights[1].falloff_start = 1.0f;
    light.lights[1].strength = XMFLOAT3(1.0f, 0.8f, 0.6f);  // 조명 색상
}

void CLightManager::Update(const CCamera* camera, const BoundingSphere& sceneBounds)
{
    light.eyePos_world = camera->GetPos();

    // Only the first "main" light casts a shadow.
    XMVECTOR lightDir = XMLoadFloat3(&Vector3::Normalize( light.lights[0].direction));
    XMVECTOR targetPos = XMLoadFloat3(&sceneBounds.Center);
    XMVECTOR lightPos = targetPos - 2.0f * sceneBounds.Radius * lightDir;
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    // Transform bounding sphere to light space
    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    float l = sphereCenterLS.x - sceneBounds.Radius;
    float b = sphereCenterLS.y - sceneBounds.Radius;
    float n = sphereCenterLS.z - sceneBounds.Radius; // Near
    float r = sphereCenterLS.x + sceneBounds.Radius;
    float t = sphereCenterLS.y + sceneBounds.Radius;
    float f = sphereCenterLS.z + sceneBounds.Radius; // Far

    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    XMMATRIX vp = lightView * lightProj;

    XMStoreFloat4x4(&light.shadow_view_proj, XMMatrixTranspose(vp));

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2(NDC to UV)
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    // Shadow Transform (World -> View -> Proj -> Texture)
    XMMATRIX S = vp * T;
    XMStoreFloat4x4(&light.shadow_transform, XMMatrixTranspose(S));

    // cube shadow
    XMVECTOR pointLightPos = XMLoadFloat3(&light.lights[1].position);

    // 시야각 90도, 종횡비 1.0f의 점 조명용 원근 투영 행렬
    XMMATRIX cubeProj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, light.lights[1].falloff_start, light.lights[1].falloff_end);

    // DX12 큐브맵 표준 축 방향 정의 (+X, -X, +Y, -Y, +Z, -Z)
    XMVECTOR cubeTargets[6] = {
        XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f), // +X
        XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f), // -X
        XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f), // +Y
        XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f), // -Y
        XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f), // +Z
        XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f)  // -Z
    };

    XMVECTOR cubeUps[6] = {
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f), // +X (Up: +Y)
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f), // -X (Up: +Y)
        XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), // +Y (Up: -Z)
        XMVectorSet(0.0f, 0.0f,  1.0f, 0.0f), // -Y (Up: +Z)
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f), // +Z (Up: +Y)
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)  // -Z (Up: +Y)
    };

    // 6개 방향에 대해 뷰 * 투영 행렬을 구해 배열에 대입
    for (int i = 0; i < 6; ++i)
    {
        XMMATRIX cubeView = XMMatrixLookAtLH(pointLightPos, pointLightPos + cubeTargets[i], cubeUps[i]);
        XMMATRIX cubeVP = cubeView * cubeProj;
        XMStoreFloat4x4(&light.cube_shadow_transforms[i], XMMatrixTranspose(cubeVP));
    }
}

void CLightManager::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
    memcpy(mapped, &light, sizeof(light));
}

void CLightManager::Render(ID3D12GraphicsCommandList* commandList)
{
    UpdateShaderVariables(commandList);

    commandList->SetGraphicsRootConstantBufferView(1, light_cb->GetGPUVirtualAddress());
}