#include "stdafx.h"
#include "AnimationController.h"
#include "AnimationManager.h"

void CAnimationController::Update(float dt)
{
    if (is_blending) {
        blend_timer += dt;
        if (blend_timer >= blend_duration) {
            // 블렌딩 완료 시점에 확실히 교체
            current_state_name = next_state_name;
            next_state_name = "";
            is_blending = false;
            blend_timer = 0.0f;
        }
    }

    // 전이 조건 체크 (블렌딩 중이 아닐 때만)
    if (!is_blending && !current_state_name.empty()) {
        auto& state = states[current_state_name];
        auto& anim = CAnimationManager::GetInstance().GetClip(state.clip_name);
        float duration = (float)anim.total_frames / 60.0f; // 60fps 기준

        // 재생 시간 누적
        current_clip_time += (dt * state.play_speed);

        // 루프 및 횟수 체크
        if (current_clip_time >= duration) {
            current_play_count++;

            // 루프 설정 확인
            if (state.is_loop && (state.max_play_count == -1 || current_play_count < state.max_play_count)) {
                // 루프 시작 시간부터 다시 시작 (loop_start_time 적용)
                current_clip_time = state.loop_start_time;
            }
            else {
                // 종료 시 마지막 프레임에 고정
                current_clip_time = duration;
            }
        }

        // 전이
        auto it = transitions.find(current_state_name);
        if (it != transitions.end()) {
            for (auto& trans : it->second) {
                // 조건이 있고 조건이 성립하면
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
    if (current_state_name == nextStateName) return;

    ResetPlayCount();
    next_state_name = nextStateName;
    blend_duration = (duration > 0.0f) ? duration : 0.001f;
    blend_timer = 0.0f;
    is_blending = true;
}