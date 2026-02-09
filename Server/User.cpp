#include "pch.h"
#include "User.h"
// ServerÂÊ User

atomic<uint64> CUser::s_idGenerator = 1;

CUser::CUser()
	: user_id(-1)
{
	user_id = s_idGenerator;
	s_idGenerator++;
}

CUser::~CUser()
{
}
