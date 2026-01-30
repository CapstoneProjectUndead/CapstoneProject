#pragma once
// ServerÂÊ GameFramework


class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	void Init();
	void Update(const float elapsedTime);

	void SendResults();
};

