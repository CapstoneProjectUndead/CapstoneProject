#pragma once
// Server쪽 Object

class CPlayer;
class Session;
class CUser;
class CComponent;

class CObject
{
    friend class CMovementComponent;
    friend class CColliderComponent;
    friend class CPhysicsManager;
    friend class CLobbyScene;

public:
	CObject(OBJECT_TYPE type);
    virtual ~CObject();

    virtual void Update(const float elapsedTime);

public:
    void                    Rotate(float pitch, float yaw, float roll);

    template<typename T>
    T*                      GetComponent() const;
    template<typename T>    
    std::vector<T*>         GetComponents() const;
    void                    SetComponent(std::shared_ptr<CComponent> component);

    uint64                  GetID() const { return obj_id; }
    void                    SetID(int id) { obj_id = id; }

    weak_ptr<Session>       GetSessionWeak() const { return session; }
    shared_ptr<Session>     GetSession() const { return session.lock(); }
    void                    SetSession(shared_ptr<Session> _session) { session = _session; }

    weak_ptr<CUser>         GetUserWeak() const { return user; }
    shared_ptr<CUser>       GetUser() const { return user.lock(); }
    void                    SetUser(shared_ptr<CUser> _user) { user = _user; }

    OBJECT_TYPE             GetObjectType() const { return obj_type; }
    void                    SetObjectType(OBJECT_TYPE type) { obj_type = type; }

    void                    SetCurrentSceneType(SCENE_TYPE type) { current_scene_type = type; }
    SCENE_TYPE              GetCurrentSceneType() const { return current_scene_type; }

    XMFLOAT3                GetPosition() { return position; }
    void                    SetPosition(const XMFLOAT3& pos) { position = pos; }
    void                    SetPosition(float x, float y, float z) { position = { x, y, z }; }

    XMFLOAT3                GetVelocity() { return velocity; }
    void                    SetVelocity(const XMFLOAT3& vel) { velocity = vel; }
    void                    SetVelocity(float vx, float vy, float vz) { velocity = { vx, vy, vz }; }

    XMFLOAT3                GetRight() { return right; }
    void                    SetRight(const XMFLOAT3 _right) { right = _right; }

    XMFLOAT3                GetUp() { return up; }
    void                    SetUp(const XMFLOAT3 _up) { up = _up; }

    XMFLOAT3                GetLook() { return look; }
    void                    SetLook(const XMFLOAT3 _look) { look = _look; }

    float                   GetYaw() const { return yaw; }

    void                    SetPitch(const float _pitch) { pitch = _pitch; }
    float                   GetPitch() const { return pitch; }

    //=================================
    // 회전 함수 (테스트)
    void SetYaw(float _yaw);
    void SetYawPitch(float yawDeg, float pitchDeg);
    void UpdateWorldMatrix();
    void UpdateLookRightFromYaw();
    //=================================

protected:
    weak_ptr<CUser>                     user;
    weak_ptr<Session>                   session;
    uint64                              obj_id;
    OBJECT_TYPE                         obj_type;
    SCENE_TYPE					        current_scene_type; // 현재 오브젝트가 속한 씬 (방이 씬을 포함하고 있는 구조)

    XMFLOAT4X4                          world_matrix;

    // world_matrix 내부 메모리를 직접 참조
    XMFLOAT3& right    =    *(XMFLOAT3*)&world_matrix._11;
    XMFLOAT3& up       =    *(XMFLOAT3*)&world_matrix._21;
    XMFLOAT3& look     =    *(XMFLOAT3*)&world_matrix._31;
    XMFLOAT3& position =    *(XMFLOAT3*)&world_matrix._41;
        
    bool                                is_visible{ true };
    bool                                is_grounded{};

    XMFLOAT3                            velocity{};
    float                               friction{ 9.0f };
    float                               jump_power{ 5.0f };


    // 회전을 쿼터니언 방식으로 하기 위한 멤버 변수 추가
    XMFLOAT4	                        orientation = { 0.f, 0.f, 0.f, 1.f };
    float		                        yaw = 0.f;
    float		                        pitch = 0.f;

    std::vector<std::shared_ptr<CComponent>> components;
};

template<typename T>
T* CObject::GetComponent() const
{
    for (auto& comp : components)
    {
        if (T* casted = dynamic_cast<T*>(comp.get()))
            return casted;
    }
    return nullptr;
}

template<typename T>
std::vector<T*> CObject::GetComponents() const
{
    std::vector<T*> result;

    for (auto& comp : components)
    {
        if (T* casted = dynamic_cast<T*>(comp.get()))
            result.push_back(casted);
    }

    return result;
}