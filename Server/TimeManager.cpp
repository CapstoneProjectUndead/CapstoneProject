#include "pch.h"
#include "TimeManager.h"

void CTimeManager::Init()
{
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&frequency));
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&prev_count)); // CPU Å¬·°
}

void CTimeManager::Update()
{
	uint64 currentCount;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	delta_time = (currentCount - prev_count) / static_cast<double>(frequency);
	prev_count = currentCount;

	frame_count++;
	frame_time += delta_time;

	if (frame_time >= 1.f)
	{
		fps = static_cast<uint32>(frame_count / frame_time);

		frame_time = 0.f;
		frame_count = 0;
	}
}