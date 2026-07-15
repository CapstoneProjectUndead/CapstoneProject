#include "stdafx.h"
#include "AnimationController.h"
#include "AnimationManager.h"

void CAnimationController::Update(float dt)
{
    // 현재 상태 시간 진행
    if (!current_state_name.empty()) {
        auto& state = states[current_state_name];
        auto& anim = CAnimationManager::GetInstance().GetClip(state.clip_name);

        if (anim.total_frames > 0) {
            // 부동소수점 오차 및 프레임 초과 방지를 위해 max_duration 계산
            // 루프가 아닌 경우 마지막 프레임 시곡점까지만 오도록 설정
            float max_duration = state.is_loop
                ? ((float)anim.total_frames / 60.0f)
                : ((float)(anim.total_frames - 1) / 60.0f);

            current_state_time += (dt * state.play_speed);

            if (current_state_time >= max_duration) {
                current_play_count++;
                if (state.is_loop && (state.max_play_count == -1 || current_play_count < state.max_play_count)) {
                    float loop_duration = (float)anim.total_frames / 60.0f;
                    current_state_time = fmod(current_state_time, loop_duration) + state.loop_start_time;
                }
                else {
                    // 단발성 액션은 max_duration(마지막 프레임 위치)에 고정
                    current_state_time = max_duration;
                }
            }
        }
    }

    // 블렌딩 중일 때 다음 상태 시간 진행 및 완료 처리
    if (is_blending) {
        blend_timer += dt;

        if (!next_state_name.empty()) {
            auto& nextState = states[next_state_name];
            auto& nextAnim = CAnimationManager::GetInstance().GetClip(nextState.clip_name);

            if (nextAnim.total_frames > 0) {
                float nextDuration = (float)nextAnim.total_frames / 60.0f;
                next_state_time += (dt * nextState.play_speed);

                if (next_state_time >= nextDuration) {
                    if (nextState.is_loop) {
                        next_state_time = fmod(next_state_time, nextDuration) + nextState.loop_start_time;
                    }
                    else {
                        next_state_time = nextDuration;
                    }
                }
            }
        }

        // 블렌딩 종료 전환
        if (blend_timer >= blend_duration) {
            current_state_name = next_state_name;
            current_state_time = next_state_time; // 전환된 시간 이관

            next_state_name = "";
            next_state_time = 0.0f;
            is_blending = false;
            blend_timer = 0.0f;
        }
    }

    // 전이 조건 체크 (블렌딩 중이 아닐 때)
    if (!is_blending && !current_state_name.empty()) {
        auto it = transitions.find(current_state_name);
        if (it != transitions.end()) {
            for (auto& trans : it->second) {
                if (trans.condition && trans.condition()) {
                    TransitionTo(trans.to_state, trans.duration);
                    break;
                }
            }
        }
    }
}

void CAnimationController::TransitionTo(const std::string& nextStateName, float duration)
{
    if (states.find(nextStateName) == states.end()) return;

    if (current_state_name == nextStateName && !is_blending) {
        return;
    }

    // 서로 다른 클립일 때만 블렌딩 시작
    next_state_name = nextStateName;
    blend_duration = (duration > 0.0f) ? duration : 0.001f;
    next_state_time = 0.0f;
    blend_timer = 0.0f;
    is_blending = true;
    current_play_count = 0;
}