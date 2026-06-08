#include "pch.h"
#include "User.h"
// Server쪽 User

atomic<uint64> CUser::s_userid_generator = 1;

CUser::CUser()
	: user_id(s_userid_generator++)
	, is_guest(false)
	, name{}
	, room_id(-1)
{

}

CUser::~CUser()
{
	player = nullptr;
}
