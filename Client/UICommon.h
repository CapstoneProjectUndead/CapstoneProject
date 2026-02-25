#pragma once
#include <string>

// 로딩 팝업 종류
enum class LoadingType
{
    None,

    // Title Scene 관련
    SignUp,
    Login,
    Logout,
    RoomCreate,
    RoomEnter,
    SinglePlay,

    // Custom Scene 관련
    SelectResult,
};

// 결과 팝업 데이터 (성공/실패 메시지)
struct ActionResult
{
    bool is_visible = false;
    bool is_success = false;
    std::string message;
};