#pragma once
// ServerÂÊ GameFramework


class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	void Init();
	void Update(float elapsedTime);

	void SendResults();
};

