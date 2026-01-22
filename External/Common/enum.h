#pragma once

enum class PLAYER_STATE : uint8_t
{
	IDLE,
	WALK,
	RUN,
};

enum class SCENE_TYPE
{
	TEST,
	MAIN,
	LOBY,
	GAME,

	END
};