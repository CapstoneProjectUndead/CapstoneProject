#pragma once

// 애니메이션 상태 정보
struct State {
    std::string name;           // 상태 이름
    std::string clip_name;      // 실제 재생할 클립 이름 (또는 블렌드 스페이스 식별자)
    float play_speed{ 1.0f };
    bool is_loop{true};
    int max_play_count{-1};
    float loop_start_time{};    // 루프가 시작될 시간
};

// 상태 전이 정보
struct Transition {
    std::string to_state;               // 목적지 상태 이름
    float duration;                     // 블렌딩 시간
    std::function<bool()> condition;    // 전이 발동 조건
};

class CAnimationController {
public:
    CAnimationController() = default;

    void ResetPlayCount() { current_play_count = 0; current_clip_time = 0.0f; }
    void AddState(const State& state) {
        states[state.name] = state;
        if (current_state_name.empty()) current_state_name = state.name;
    }
    void AddTransition(const std::string& from, const Transition& transition) {
        transitions[from].push_back(transition);
    }

    void Update(float dt);
    void TransitionTo(const std::string& nextStateName, float duration);

    // Getter
    std::string GetCurrentState() const { return current_state_name; }
    int GetPlayCount() const { return current_play_count; }
    float GetCurrentClipTime() const { return current_clip_time; }
    bool IsBlending() const { return is_blending; }
    // 0~1 사이값
    float GetWeight() const { return is_blending ? (blend_timer / blend_duration) : 0.0f; }

    std::string GetCurrentClip() const {
        return states.at(current_state_name).clip_name;
    }
    std::string GetNextClip() const {
        if (!is_blending) return "";
        return states.at(next_state_name).clip_name;
    }
private:
    std::string current_state_name;
    std::string next_state_name;

    std::map<std::string, State> states;
    std::map<std::string, std::vector<Transition>> transitions;

    float blend_timer{};
    float blend_duration{};
    bool is_blending{};

    float current_clip_time{}; // 현재 애니메이션 재생 시간
    int current_play_count{};
};