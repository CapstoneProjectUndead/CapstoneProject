#pragma once

class CJitterMeasurer
{
private:
    float last_arrival_time = 0.0f;         // 이전 패킷 도착 시간
    float expected_interval = 1.0f / 60.0f; // 기대하는 간격 (60Hz 서버라면 0.0166s)

    std::deque<float> jitter_sample_deq;    // 최근 지터 값들을 담을 버퍼
    const size_t MAX_SAMPLES = 60;          // 최근 60개 패킷(약 1초)의 평균을 계산

    float current_jitter = 0.0f;            // 최종 계산된 지터 평균값

public:
    // 패킷이 도착했을 때 호출
    void OnPacketArrival(float currentTime)
    {
        if (last_arrival_time > 0.0f)
        {
            // 1. 실제 도착 간격 계산
            float actualInterval = currentTime - last_arrival_time;

            // 2. 지터 계산: |실제 간격 - 기대 간격|
            // 패킷이 16.6ms보다 늦게 오거나 일찍 올 때의 변동폭을 구함
            float jitter = std::abs(actualInterval - expected_interval);

            // 3. 샘플 버퍼에 저장
            jitter_sample_deq.push_back(jitter);
            if (jitter_sample_deq.size() > MAX_SAMPLES)
                jitter_sample_deq.pop_front();

            // 4. 이동 평균(Moving Average) 계산
            float sum = std::accumulate(jitter_sample_deq.begin(), jitter_sample_deq.end(), 0.0f);
            current_jitter = sum / jitter_sample_deq.size();
        }

        last_arrival_time = currentTime;
    }

    // 보간 시스템이 참조할 현재 지터값 반환
    float GetCurrentJitter() const { return current_jitter; }
};

