#pragma once
// Server쪽 CustomScene
#include "Scene.h"


class CCustomScene :
    public CScene
{
public:
    CCustomScene();
    CCustomScene(uint32 roomId);
    ~CCustomScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

public:
    void C_Enter_CustomScene(shared_ptr<Session> session, const PktDummy& pkt);
    void C_Handle_Custom_Select(shared_ptr<Session> session, const C_CustomSelect& pkt);

private:
    // 맵의 바닥, 장애물 등 움직이지 않는 정적 충돌체들을 보관하는 곳
    std::vector<std::shared_ptr<CObject>> static_objects;
};

