#pragma once

class CScene;
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

