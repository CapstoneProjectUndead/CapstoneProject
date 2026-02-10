#include "stdafx.h"
#include "User.h"
#include "MyPlayer.h"

CUser::CUser()
	: user_id(-1)
{

}

CUser::~CUser()
{
	my_player = nullptr;
}
