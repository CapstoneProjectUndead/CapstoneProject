#include "stdafx.h"
#include "LightManager.h"
#include "Camera.h"

void CLightManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    light_cb = CreateBufferResource(
        device,
        commandList,
        &light,
        CalculateConstant<LightCB>(),
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr
    );

}

void CLightManager::Update(const CCamera* camera)
{
    light.eyePosWorld = camera->GetPos();
    light.ambientLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

    // 방향광 3개 예시
    light.lights[0].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
    light.lights[0].strength = XMFLOAT3(1.0f, 1.0f, 1.0f);

    light.lights[1].direction = XMFLOAT3(-0.577f, -0.577f, 0.0f);
    light.lights[1].strength = XMFLOAT3(0.5f, 0.5f, 0.5f);

    light.lights[2].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    light.lights[2].strength = XMFLOAT3(0.3f, 0.3f, 0.3f);

    UINT8* mapped = nullptr;
    light_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    memcpy(mapped, &light, sizeof(light));
    light_cb->Unmap(0, nullptr);

}

void CLightManager::Render(ID3D12GraphicsCommandList* commandList)
{
    commandList->SetGraphicsRootConstantBufferView(3, light_cb->GetGPUVirtualAddress());
}
