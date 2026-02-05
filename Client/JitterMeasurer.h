#pragma once

class CJitterMeasurer
{
private:
    float               last_arrival_time = 0.0f;           // 이전 패킷 도착 시간
    const float         expected_interval = 1.0f / 60.0f;   // 기대하는 간격 (60Hz 서버라면 0.0166s)
    float               avg_interval = expected_interval;   // 기본값 (60Hz)
    float               current_jitter = 0.0f;              // 최종 계산된 지터 평균값

    // 지수 이동 평균(EMA) 계수 (0.9 ~ 0.99 사이 권장)
    // 0.9는 반응이 빠르고, 0.99는 수치가 아주 안정적입니다.
    const float         alpha = 0.1f;

    uint64              current_sample = 0;
    const size_t        MAX_SAMPLES = 60;                   // 최근 60개 패킷(약 1초)의 평균을 계산
    std::deque<float>   jitter_sample_deq;                  // 최근 지터 값들을 담을 버퍼

public:
    // 패킷이 도착했을 때 호출
    void OnPacketArrival(float currentTime)
    {
        ++current_sample;

        // 1. 첫 패킷 처리: 기준 시간만 잡고 나감
        if (last_arrival_time <= 0.0f) {
            last_arrival_time = currentTime;
            return;
        }

        // 2. 시간 역전 및 동시 도착 방지
        // 시뮬레이터에서 아주 미세한 오차로 시간이 같거나 뒤로 갈 경우 계산 제외
        if (currentTime <= last_arrival_time) {
            return;
        }

        float actualInterval = currentTime - last_arrival_time;

        // 3. 상황별 유효 범위 필터링
#ifdef GENERATE_LAG
        // 렉 시뮬레이션 중에는 지터가 큼을 감안하여 범위를 넓게 잡음
        const float MAX_REASONABLE = 3.0f;
#else
        // 일반 상황에서는 0.3초 이상의 튀는 값은 무시하여 평균 오염 방지
        const float MAX_REASONABLE = 0.3f;
#endif

        if (actualInterval > 0.0f && actualInterval < MAX_REASONABLE) {

            // 4. 평균 간격 업데이트 (EMA)
            // 지연이 발생하면 이 값이 0.016에서 점차 커집니다.
            avg_interval = (avg_interval * (1.0f - alpha)) + (actualInterval * alpha);

            // 5. 지터 계산 (실제 간격과 평균 간격의 편차)
            float jitter = std::abs(actualInterval - avg_interval);
            current_jitter = (current_jitter * (1.0f - alpha)) + (jitter * alpha);
        }

        // 마지막 도착 시간 갱신
        last_arrival_time = currentTime;
    }

    int GetSampleCount() const { return current_sample; }

    float GetAverageInterval() const { return avg_interval; }

    // 보간 시스템이 참조할 현재 지터값 반환
    float GetCurrentJitter() const { return current_jitter; }

    void Reset() 
    {
        last_arrival_time = 0.0f;
        avg_interval = 0.0166f;
        current_jitter = 0.0f;
    }
};

