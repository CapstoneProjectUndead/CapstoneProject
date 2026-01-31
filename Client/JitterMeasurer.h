#pragma once

class CJitterMeasurer
{
private:
    float       last_arrival_time = 0.0f;           // 이전 패킷 도착 시간
    const float expected_interval = 1.0f / 60.0f;   // 기대하는 간격 (60Hz 서버라면 0.0166s)
    float       avg_interval = expected_interval;   // 기본값 (60Hz)


    std::deque<float> jitter_sample_deq;    // 최근 지터 값들을 담을 버퍼
    const size_t MAX_SAMPLES = 60;          // 최근 60개 패킷(약 1초)의 평균을 계산

    float current_jitter = 0.0f;            // 최종 계산된 지터 평균값

public:
    // 패킷이 도착했을 때 호출
    void OnPacketArrival(float currentTime)
    {
        if (last_arrival_time > 0.0f) {
            float actualInterval = currentTime - last_arrival_time;

            // 1. 패킷 사이의 평균 간격을 업데이트 (지수 이동 평균)
            // 서버가 0.5초마다 보낸다면 이 값이 서서히 0.5로 수렴합니다.
            avg_interval = (avg_interval * 0.9f) + (actualInterval * 0.1f);

            // 2. 지터 계산 (실제 간격과 평균 간격의 차이)
            float jitter = std::abs(actualInterval - avg_interval);
            current_jitter = (current_jitter * 0.9f) + (jitter * 0.1f);
        }

        last_arrival_time = currentTime;
    }

    float GetAverageInterval() const { return avg_interval; }

    // 보간 시스템이 참조할 현재 지터값 반환
    float GetCurrentJitter() const { return current_jitter; }
};

