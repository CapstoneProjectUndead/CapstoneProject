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
    light.lights[0].direction = XMFLOAT3(0 , -1, 0);
    XMVECTOR dir = XMVector3Normalize(XMVectorSet(0.5f, -1.0f, 0.5f, 0.0f));
    XMStoreFloat3(&light.lights[0].direction, dir);
    light.lights[0].strength = XMFLOAT3(1.0f, 1.0f, 1.0f);

    //light.lights[1].direction = XMFLOAT3(-0.577f, -0.577f, 0.0f);
    //light.lights[1].strength = XMFLOAT3(0.5f, 0.5f, 0.5f);

    //light.lights[2].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    //light.lights[2].strength = XMFLOAT3(0.3f, 0.3f, 0.3f);
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