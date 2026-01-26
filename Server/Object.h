#pragma once
// Server쪽 Object

class CPlayer;
class Session;

class CObject
{
public:
	CObject();
    virtual ~CObject();

    virtual void Update(float elapsedTime);

public:
    static shared_ptr<CPlayer>          CreatePlayer();

    void                                Move(const XMFLOAT3& direction, float distance);
    void                                Move(const XMFLOAT3& shift);
    void                                Rotate(float pitch, float yaw, float roll);

    int                                 GetID() const { return obj_id; }
    void                                SetID(int id) { obj_id = id; }

    weak_ptr<Session>                   GetSessionWeak() const { return session; }
    shared_ptr<Session>                 GetSession() const { return session.lock(); }
    void                                SetSession(shared_ptr<Session> _session) { session = _session; }

    XMFLOAT3                            GetPosition() { return position; }
    void                                SetPosition(const XMFLOAT3& pos) { position = pos; }
    void                                SetPosition(float x, float y, float z) { position = { x, y, z }; }

    XMFLOAT3                            GetRight() { return right; }
    void                                SetRight(const XMFLOAT3 _right) { right = _right; }

    XMFLOAT3                            GetUp() { return up; }
    void                                SetUp(const XMFLOAT3 _up) { up = _up; }

    XMFLOAT3                            GetLook() { return look; }
    void                                SetLook(const XMFLOAT3 _look) { look = _look; }

    float                               GetYaw() const { return yaw; }
    float                               GetPitch() const { return pitch; }

    //=================================
    // 회전 함수 (테스트)
    void SetYaw(float _yaw);
    void SetYawPitch(float yawDeg, float pitchDeg);
    void UpdateWorldMatrix();
    void UpdateLookRightFromYaw();
    //=================================

private:
    static atomic<uint64>               s_idGenerator;
    weak_ptr<Session>                   session;
    uint64                              obj_id;

    XMFLOAT4X4                          world_matrix;

    // world_matrix 내부 메모리를 직접 참조
    XMFLOAT3& right    =    *(XMFLOAT3*)&world_matrix._11;
    XMFLOAT3& up       =    *(XMFLOAT3*)&world_matrix._21;
    XMFLOAT3& look     =    *(XMFLOAT3*)&world_matrix._31;
    XMFLOAT3& position =    *(XMFLOAT3*)&world_matrix._41;
         
    float                               speed{ 10.f };
    bool                                is_visible{ true };

    // 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
    XMFLOAT4	                        orientation = { 0.f, 0.f, 0.f, 1.f };
    float		                        yaw = 0.f;
    float		                        pitch = 0.f;

    //BoundingOrientedBox oobb; // 충돌은 서버 핵심
};

