#include "stdafx.h"
#include "AnimationController.h"

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

    next_state_name = nextStateName;
    blend_duration = (duration > 0.0f) ? duration : 0.001f;
    blend_timer = 0.0f;
    is_blending = true;
}