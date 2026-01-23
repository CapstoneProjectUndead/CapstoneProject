#pragma once

class CTimeManager
{
private:
	CTimeManager() = default;
	CTimeManager(const CTimeManager&) = delete;

public:
	~CTimeManager() {};

	static CTimeManager& GetInstance() {
		static CTimeManager instance;
		return instance;
	}

public:
	void	Init();
	void	Update();

	double	GetDeltaTime() { return delta_time; }
	double	GetClampedDeltaTime(double maxDT = 0.25) const
	{
		return (delta_time > maxDT) ? maxDT : delta_time;
	}

	uint32	GetFps() const { return fps; }

private:
	uint64	frequency = 0;
	uint64	prev_count = 0;
	double	delta_time = 0.f;

private:
	uint32	frame_count = 0;
	double	frame_time = 0.f;
	uint32	fps = 0;
};

