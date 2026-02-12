#pragma once
#include "Room.h"

class CUser;

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

public:
    uint32  CreateRoom(const string& name, shared_ptr<CUser> user);
    void    DestroyRoomLock(uint32 roomId);
    void    DestroyRoomNoLock(uint32 roomId);
    CRoom*  FindRoom(uint32 roomId);

    const unordered_map<uint32, unique_ptr<CRoom>>& GetRooms() const { return rooms; }
    mutex& GetMutex() { return rooms_lock; }

private:
    mutex rooms_lock;
    unordered_map<uint32, unique_ptr<CRoom>> rooms;
};

