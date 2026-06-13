#include "stdafx.h"
#include "LightManager.h"
#include "Camera.h"

void CLightManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    light.ambient_light = XMFLOAT4(0.05f, 0.08f, 0.15f, 1.0f);
    light.active_dot_num = 0; // 초기에는 점 조명 0개

    // 0번은 무조건 방향성 조명(시작 조명)
    light.lights[0].direction = XMFLOAT3(0.01f, -1.0f, 0.01f);
    light.lights[0].strength = XMFLOAT3(0.5f, 0.6f, 0.75f);

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
}

// 기존 점조명 리셋
void CLightManager::ClearPointLights()
{
    light.active_dot_num = 0;
    for (UINT i = 1; i < MaxLights; ++i)
    {
        light.lights[i] = Light{};
    }
}

// 새로운 점조명 추가 API
bool CLightManager::AddPointLight(const XMFLOAT3& position, const XMFLOAT3& strength, float falloffStart, float falloffEnd)
{
    // 최대 개수(15개)를 초과하면 등록 취소
    if (light.active_dot_num >= MAX_POINT_LIGHTS)
        return false;

    UINT nextIdx = light.active_dot_num + 1;

    light.lights[nextIdx].position = position;
    light.lights[nextIdx].strength = strength;
    light.lights[nextIdx].falloff_start = falloffStart;
    light.lights[nextIdx].falloff_end = falloffEnd;

    light.active_dot_num++;
    return true;
}

void CLightManager::Update(const CCamera* camera, const BoundingSphere& sceneBounds)
{
    light.eyePos_world = camera->GetPos();

    XMVECTOR lightDir = XMLoadFloat3(&Vector3::Normalize(light.lights[0].direction));
    XMVECTOR targetPos = XMLoadFloat3(&sceneBounds.Center);
    XMVECTOR lightPos = targetPos - 2.0f * sceneBounds.Radius * lightDir;
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    float l = sphereCenterLS.x - sceneBounds.Radius;
    float b = sphereCenterLS.y - sceneBounds.Radius;
    float n = sphereCenterLS.z - sceneBounds.Radius;
    float r = sphereCenterLS.x + sceneBounds.Radius;
    float t = sphereCenterLS.y + sceneBounds.Radius;
    float f = sphereCenterLS.z + sceneBounds.Radius;

    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
    XMMATRIX vp = lightView * lightProj;
    XMStoreFloat4x4(&light.shadow_view_proj, XMMatrixTranspose(vp));

    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);
    XMMATRIX S = vp * T;
    XMStoreFloat4x4(&light.shadow_transform, XMMatrixTranspose(S));

    // 큐브 섀도우 맵 매트릭스 갱신 (현재 등록된 active_dot_num 만큼만 루프)
    const XMVECTOR cubeTargets[6] = {
        XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f), XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f),
        XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
        XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f), XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f)
    };
    const XMVECTOR cubeUps[6] = {
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f),
        XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), XMVectorSet(0.0f, 0.0f,  1.0f, 0.0f),
        XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)
    };

    for (UINT i = 1; i <= light.active_dot_num; ++i) {
        XMVECTOR pointLightPos = XMLoadFloat3(&light.lights[i].position);
        XMMATRIX cubeProj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, light.lights[i].falloff_start, light.lights[i].falloff_end);

        for (int j = 0; j < 6; ++j) {
            XMMATRIX cubeView = XMMatrixLookAtLH(pointLightPos, pointLightPos + cubeTargets[j], cubeUps[j]);
            XMMATRIX cubeVP = cubeView * cubeProj;
            XMStoreFloat4x4(&light.cube_shadow_transforms[i - 1][j], XMMatrixTranspose(cubeVP));
        }
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