#pragma once

class CPlayer;
class CMonster;
class CObject;
class Session;
class CUser;
class CRoom;
class CPhysicsManager;
namespace CGeometryLoader {
    struct FrameNode;
}
enum EColLayer : uint32_t;

class CServerObjectFactory
{
public:
    CServerObjectFactory();
    ~CServerObjectFactory();

public:
    // �׽�Ʈ��
    static shared_ptr<CPlayer> CreatePlayerTest(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CPhysicsManager> physicsManager);

    static shared_ptr<CPlayer> CreatePlayer(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager);
    static shared_ptr<CMonster> CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager);
    static void InitializeCharacter(const std::string& fileName, shared_ptr<CObject>& object, shared_ptr<CPhysicsManager>& physicsManager, bool isPlayer);
    static void InitializeUndeadCharacter(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager);
    static void InitializeHumanMonster(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager);
    static void InitializeGhost(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager);

    static void ProcessNode(shared_ptr<CPhysicsManager> physicsManager, shared_ptr<CObject>& object, const std::unique_ptr<CGeometryLoader::FrameNode>& node, bool isPlayer);
    static void AddCollider(shared_ptr<CPhysicsManager> physicsManager, std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EColLayer category, EColLayer mask);
private:
    static atomic<uint32> monster_id_generator;
};
