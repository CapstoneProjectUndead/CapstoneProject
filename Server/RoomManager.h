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
    void    Update(const float elapsedTime);
    void    SendResults();
    uint32  CreateRoom(const string& name, shared_ptr<CUser> user);

    map<uint32, unique_ptr<CRoom>>& GetRooms() { return rooms; }

private:
    mutex rooms_lock;
    map<uint32, unique_ptr<CRoom>> rooms;
};

