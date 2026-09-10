#include "stdafx.h"
#include "Timer.h"

CTimer::CTimer()
{
	::timeBeginPeriod(1);

	// 성능 주파수를 사용할 수 없으면 멀티 미디어 타이머 사용
	if (QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frequency)) {
		used_performance_counter = true;
		QueryPerformanceCounter((LARGE_INTEGER*)&last_performance_counter);
		time_sacle = 1.0 / (double)performance_frequency;
	}
	else {
		used_performance_counter = false;
		last_performance_counter = timeGetTime();
		time_sacle = 0.001f;	// miilisecond
	}
}

CTimer::~CTimer()
{
	::timeEndPeriod(1);
}

void CTimer::Tick(float lockFps)
{
	if (is_stop)
	{
		time_elapsed = 0.0f;
		return;
	}
	
	if (used_performance_counter)
		::QueryPerformanceCounter((LARGE_INTEGER*)&current_performance_counter);
	else
		current_performance_counter = timeGetTime();

	float timeElapse = float((current_performance_counter - last_performance_counter) * time_sacle);

	if (lockFps > 0.0f)
	{
		const float targetDelta = 1.0f / lockFps;

		// 원하는 delta time이 될 때까지 대기
		while (timeElapse < targetDelta)
		{
			float remaining = targetDelta - timeElapse;
			// 남은 시간이 2ms 이상이면 Sleep(1)로 CPU 과점유 방지
			if (remaining > 0.002f)
			{
				::Sleep(1);
			}
			else
			{
				// 2ms 이하로 남았을 때는 고정밀 스핀락(YieldProcessor)으로 정확한 타이밍 맞춤
				::YieldProcessor();
			}

			if (used_performance_counter)
				::QueryPerformanceCounter((LARGE_INTEGER*)&current_performance_counter);
			else
				current_performance_counter = timeGetTime();

			timeElapse = float((current_performance_counter - last_performance_counter) * time_sacle);
		}
	}

	last_performance_counter = current_performance_counter;

	// 마지막 프레임 처리 시간과 현재 프레임 처리 시간의 차이가 1초보다 작으면 현재 프레임 처리 시간을 frame_time[0]에 저장
	if (fabsf(timeElapse - time_elapsed) < 1.0f)
	{
		::memmove(&frame_time[1], frame_time, (kMaxSampleCount - 1) * sizeof(float));
		frame_time[0] = timeElapse;
		if (sample_cnt < kMaxSampleCount) ++sample_cnt;
	}

	// 프레임++, 현재 프레임 처리 시간을 누적 하고 저장
	++frame_per_seconds;
	fps_time_elapsed += timeElapse;
	if (fps_time_elapsed > 1.0f) {
		current_frame_rate = static_cast<size_t>(frame_per_seconds);
		frame_per_seconds = 0;
		fps_time_elapsed = 0.0f;
	}

	// 샘플링 및 delta time 결정
	if (lockFps > 0.0f)
	{
		// 프레임 고정 모드일 때는 물리 진동 방지를 위해 고정 delta time(1/lockFps) 공급
		time_elapsed = 1.0f / lockFps;
	}
	else
	{
		time_elapsed = 0.0f;
		for (ULONG i = 0; i < sample_cnt; i++) time_elapsed += frame_time[i];
		if (sample_cnt > 0) time_elapsed /= sample_cnt;
	}
}

size_t CTimer::GetFrameRate(LPTSTR title, int frame)
{
	if (title) {
		_itow_s(static_cast<int>(current_frame_rate), title, frame, 10);
		wcscat_s(title, frame, _T(" FPS)"));
	}

	return current_frame_rate;
}

void CTimer::Reset()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&last_performance_counter);

	current_performance_counter = last_performance_counter;

	is_stop = false;
}
