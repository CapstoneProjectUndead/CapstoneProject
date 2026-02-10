#pragma once
// ¼­¹öÂÊ SceneManager

#include "Scene.h"

class CTitleScene;
class CUser;

class CRoom
{
    using SceneArray = array<unique_ptr<CScene>, (UINT)SCENE_TYPE::END>;
public:
    CRoom(string name);
    ~CRoom();

    SceneArray& GetScenes() { return scenes; }
    uint32 GetRoomID() const { return room_id; }
    const string& GetRoomName() const { return room_name; }
    uint16 GetTotalPlayer() const { return total_player; }

private:
    static atomic<uint64> s_room_id_generator;

    uint32      room_id;
    string      room_name;
    uint16      total_player;
    SceneArray  scenes;
};


class CSceneManager
{
private:
    CSceneManager();
    CSceneManager(const CSceneManager&) = delete;

public:
    ~CSceneManager();

    static CSceneManager& GetInstance() {
        static CSceneManager instance;
        return instance;
    }

public:
    void    Initialize();
    void    Update(const float elapsedTime);
    void    SendResults();

#ifdef SCENE_TEST
    unique_ptr<CScene>* GetScenes() { return scenes; }
#endif

    CTitleScene* GetTitleScene() const { return title_scene.get(); }
    map<uint32, unique_ptr<CRoom>>& GetRooms() { return rooms; }

    void CreateRoom(const string& name, shared_ptr<CUser> user);

private:
#ifdef SCENE_TEST
    unique_ptr<CScene>      scenes[(UINT)SCENE_TYPE::END];
#endif

    unique_ptr<CTitleScene> title_scene;
    map<uint32, unique_ptr<CRoom>> rooms;
};

