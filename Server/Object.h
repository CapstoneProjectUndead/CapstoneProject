#pragma once
// Server쪽 Object

struct Vec3 
{ 
    float x, y, z; 
};

class CPlayer;
class Session;

class CObject
{
public:
	CObject();
    virtual ~CObject();

    virtual void Update(float elapsedTime);

public:
    static shared_ptr<CPlayer> CreatePlayer();

    void SetPosition(float x, float y, float z)
    {
        position = { x, y, z };
    }

    void Move(const Vec3& dir, float distance)
    {
        position.x += dir.x * distance;
        position.y += dir.y * distance;
        position.z += dir.z * distance;
    }

    int  GetID() const { return obj_id; }
    void SetID(int id) { obj_id = id; }

    weak_ptr<Session>   GetSessionWeak() const { return session; }
    shared_ptr<Session> GetSession() const { return session.lock(); }
    void                SetSession(shared_ptr<Session> _session) { session = _session; }

    Vec3 GetPosition() { return position; }
    void SetPosition(Vec3 pos) { position = pos; }

    Vec3 GetRight() { return position; }
    void SetRight(Vec3 _right) { right = _right; }

    Vec3 GetUp() { return position; }
    void SetUp(Vec3 _up) { up = _up; }

    Vec3 GetLook() { return position; }
    void SetLook(Vec3 _look) { look = _look; }

private:
    static atomic<int> s_idGenerator;
    weak_ptr<Session>  session;

    int obj_id;

    Vec3 right{ 1.f, 0.f, 0.f };    // 11
    Vec3 up{ 0.f, 1.f, 0.f };       // 21
    Vec3 look{ 0.f, 0.f, 1.f };     // 31
    Vec3 position{0.f, 0.f, 0.f};   // 41
         
    float speed{ 10.f };

    //BoundingOrientedBox oobb; // 충돌은 서버 핵심
};

