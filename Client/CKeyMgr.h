#pragma once

struct Vec2
{
    float x = 0.f;
    float y = 0.f;

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    Vec2(const DirectX::XMFLOAT2& f) : x(f.x), y(f.y) {}

    operator DirectX::XMFLOAT2() const { return { x, y }; }

    Vec2 operator-(const Vec2& rhs) const
    {
        XMVECTOR a = XMVectorSet(x, y, 0.f, 0.f);
        XMVECTOR b = XMVectorSet(rhs.x, rhs.y, 0.f, 0.f);
        XMVECTOR r = XMVectorSubtract(a, b);

        Vec2 out;
        XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(&out), r);
        return out;
    }

    void Normalize()
    {
        XMVECTOR v = XMVectorSet(x, y, 0.f, 0.f);
        v = XMVector2Normalize(v);
        XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(this), v);
    }
};

enum class KEY
{
    W, S, A, D,
    Z, X, C, V,
    
    R, T, Y, U, I, O, P,

    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,

    NUM0, NUM1, NUM2, NUM3, NUM4,
    NUM5, NUM6, NUM7, NUM8, NUM9,

    LEFT,
    RIGHT,
    UP,
    DOWN,

    LBTN,
    RBTN,

    ENTER,
    ESC,
    SPACE,
    LSHIFT,
    ALT,
    CTRL,

    KEY_END
};

extern UINT gKeyValue[(UINT)KEY::KEY_END];

enum class KEY_STATE
{
    TAP,
    PRESSED,
    RELEASED,
    NONE
};

struct tKeyInfo
{
    KEY_STATE   State;
    bool        PrevPressed;

    tKeyInfo() = default;

    tKeyInfo(KEY_STATE state, bool pressed)
        : State(state)
        , PrevPressed(pressed)
    {
    }
};

class CKeyMgr
{
private:
    CKeyMgr() {};
    CKeyMgr(const CKeyMgr&) = delete;

public:
    ~CKeyMgr() {};

    static CKeyMgr& GetInstance() {
        static CKeyMgr instance;
        return instance;
    }

private:
    std::vector<tKeyInfo>   input_vector;
    Vec2                    cur_mouse_pos;
    Vec2                    prev_mouse_pos;
    Vec2                    drag_dir;

public:
    void init();
    void tick();

public:
    KEY_STATE GetKeyState(KEY _Key) { return input_vector[(UINT)_Key].State; }

    Vec2 GetMousePos() { return cur_mouse_pos;}
    Vec2 GetPrevMousePos() { return prev_mouse_pos; }
    Vec2 GetMouseDrag() { return drag_dir; }
};

