#pragma once
// Server쪽 GameFramework


class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	void Init();
	void Update();

public:
    // 메인 루프(Update)에서 호출
	//void HandlePackets();

private:
	mutex					queue_mutex;
	queue<C_PlayerInput>	input_queue;
};

