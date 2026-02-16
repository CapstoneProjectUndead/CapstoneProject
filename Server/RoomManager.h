#pragma once
#include "Room.h"

class CUser;
class CPlayer;

enum ROOM_EVENT_TYPE : uint8_t
{
    Create,
    Destroy,
    Enter
};

class CRoomManager
{
private:
    CRoomManager();
    CRoomManager(const CRoomManager&) = delete;

public:
    ~CRoomManager();

    static CRoomManager& GetInstance() {
        static CRoomManager instance;
        return instance;
    }

public:
    void    Initialize();
    void    Update(const float elapsedTime);
    void    SendResults();

    void    CheckEmptyRoom();
    void    DeActiveRoom(uint32 roomId) { rooms[roomId]->SetActive(false); }
    void    DeActiveRoom(shared_ptr<CRoom> room) {room->SetActive(false); }

public:
    void                CreateRoom(const string& name, shared_ptr<CUser> user);
    shared_ptr<CRoom>   FindRoomLock(uint32 roomId);
    shared_ptr<CRoom>   FindRoomNoLock(uint32 roomId);
    void                DestroyRoomLock(uint32 roomId);
    void                DestroyRoomNoLock(uint32 roomId);
    void                EnterRoom(shared_ptr<Session> session, uint32 roomId);
    void                LeaveAndCleanupRoom(shared_ptr<CPlayer> player);
    void                SendRoomList(shared_ptr<Session> session);

    const unordered_map<uint32, shared_ptr<CRoom>>& GetRooms() const { return rooms; }
    unordered_map<uint32, shared_ptr<CRoom>>& GetRooms() { return rooms; }

public:
    template<typename... T>
    void ReserveEvent(ROOM_EVENT_TYPE type, T&&... args)
    {
        events.push(
            [this, type, ...args = std::forward<T>(args)]() mutable
            {
                switch (type)
                {
                case ROOM_EVENT_TYPE::Create:
                    CreateRoom(args...);
                    break;
                }
            }
        );
    }

    void ProcessEvents();

private:
    mutex rooms_lock;
    unordered_map<uint32, shared_ptr<CRoom>> rooms;

    std::queue<std::function<void()>> events;
};

