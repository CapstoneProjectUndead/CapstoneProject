#pragma once
#include <string>

// 로딩 팝업 종류
enum class LoadingType
{
    None,
    SignUp,
    Login,
    Logout,
    RoomCreate,
    RoomEnter,
    SinglePlay
};

// 결과 팝업 데이터 (성공/실패 메시지)
struct ActionResult
{
    bool is_visible = false;
    bool is_success = false;
    std::string message;

    void Success(const std::string& _message)
    {
        is_visible = true;
        is_success = true;
        message = _message;
    }

    void Fail(const std::string& _message)
    {
        is_visible = true;
        is_success = false;
        message = _message;
    }
};