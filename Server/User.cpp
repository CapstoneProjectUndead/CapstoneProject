#include "pch.h"
#include "User.h"
// ServerÂÊ User

atomic<uint64> CUser::s_userid_generator = 1;

CUser::CUser()
	: user_id(s_userid_generator++)
	, name{}
	, room_id(-1)
{

}

CUser::~CUser()
{
	player = nullptr;
}
