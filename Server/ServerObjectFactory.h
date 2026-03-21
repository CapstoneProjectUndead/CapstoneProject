#pragma once

class CPlayer;
class CMonster;
class CObject;
class Session;
class CUser;
class CRoom;
class CPhysicsManager;


class CServerObjectFactory
{
public:
    CServerObjectFactory();
    ~CServerObjectFactory();

public:
    // 테스트용
    static shared_ptr<CPlayer> CreatePlayerTest(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CPhysicsManager> physicsManager);

    static shared_ptr<CPlayer> CreatePlayer(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager);
    static shared_ptr<CMonster> CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager);
    static void InitializeCharacter(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager);

private:
    static atomic<uint32> monster_id_generator;
};
