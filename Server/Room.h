#pragma once

class CScene;
class CUser;

class CRoom : public enable_shared_from_this<CRoom>
{
    using SceneArray = array<unique_ptr<CScene>, (UINT)SCENE_TYPE::END>;
public:
    CRoom(RoomInfo roomInfo);
    CRoom(string name);
    ~CRoom();

    void Initialize();
    void Update(const float elapsedTime);
    void SendResults();

public:
    void PlayerEnter() { ++room_info.current_player_count; }
    void PlayerLeave() { --room_info.current_player_count; }

public:
    SceneArray&     GetScenes() { return scenes; }

    RoomInfo        GetRoomInfo() const { return room_info; }

    uint32          GetRoomID() const { return room_info.room_id; }
    const string&   GetRoomName() const { return string(room_info.room_name); }
    uint16          GetCurrentPlayerCount() const { return room_info.current_player_count; }
    bool            GetIsGameStart() const { return room_info.is_game_start; }

    bool            IsValid();
    bool            IsActive() const { return is_active; }
    void            SetActive(bool active) { is_active = active; }

private:
    static atomic<uint32> s_room_id_generator;

    bool        is_active;
    RoomInfo    room_info;
    SceneArray  scenes;
    mutex       room_lock;
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