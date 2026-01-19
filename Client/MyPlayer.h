#pragma once
#include "Player.h"

class CMyPlayer :
    public CPlayer
{
public:
    using CObject::Move; // 부모의 Move 모두 노출

    CMyPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    ~CMyPlayer();

    virtual void Update(float elapsedTime) override;

    void ProcessInput();

    CCamera* GetCameraPtr() const { return camera.get(); }

protected:
    std::shared_ptr<CCamera> camera;
};

