#pragma once

class CScene;
class CUser;

class CRoom
{
    using SceneArray = array<unique_ptr<CScene>, (UINT)SCENE_TYPE::END>;
public:
    CRoom(RoomInfo roomInfo);
    CRoom(string name);
    ~CRoom();

    void Update(const float elapsedTime);
    void SendResults();

public:
    SceneArray&     GetScenes() { return scenes; }

    RoomInfo        GetRoomInfo() const { return room_info; }

    uint32          GetRoomID() const { return room_info.room_id; }
    const string&   GetRoomName() const { return string(room_info.room_name); }
    uint16          GetCurrentPlayerCount() const { return room_info.current_player_count; }
    bool            GetIsGameStart() const { return room_info.is_game_start; }

    bool            SearchPlayersAllScene();

private:
    static atomic<uint32> s_room_id_generator;

    RoomInfo    room_info;
    SceneArray  scenes;
};

/*
struct RoomInfo
{
	uint16	room_id;	// 방 ID
	char	room_name[ROOM_NAME_MAX]; // 100자
	uint16	current_player_count;
	bool	is_game_start;	// 게임이 이미 시작된 방인지
};
*/