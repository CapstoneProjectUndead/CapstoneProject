#pragma once
//======================================
// **** 클라/서버 공동 참조 헤더 파일 ****
//======================================

enum class PLAYER_STATE : uint8_t
{
	IDLE,
	WALK,
	RUN,
};

enum class SCENE_TYPE
{
	TITLE,
	LOBBY,
	GAME,

	END
};