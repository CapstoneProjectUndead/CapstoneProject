#pragma once
// Server쪽 TestScene
#include "Scene.h"

class CUser;

class CLobbyScene :
    public CScene
{
public:
    CLobbyScene(uint32 roomId);
    ~CLobbyScene();

    virtual void Start() override;
    virtual void Update(float elapsedTime) override;

public:
    //=================
    // 테스트용 함수
    void C_Enter_Player(shared_ptr<Session> session, const C_LOGIN& pkt);
    //=================

private:
    enum class LobbyMeshName {
        Wall,
        Floor,
        GroundPipe,
        Unknown
    };

    LobbyMeshName stringToLobbyMeshName(const std::string& str);

    void CreateLobby();

private:
    // 맵의 바닥, 장애물 등 움직이지 않는 정적 충돌체들을 보관하는 곳
    std::vector<std::shared_ptr<CObject>> static_objects;
};

