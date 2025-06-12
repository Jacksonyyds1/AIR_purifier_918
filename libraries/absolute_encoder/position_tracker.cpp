#include "absolute_encoder.h"
#include "position_tracker.hpp"
#include <algorithm>

namespace AbsoluteEncoder
{

PositionTracker::PositionTracker(EncoderMap &encoder_map, const uint8_t *pattern,
                                    uint16_t pattern_length)
    : encoder_map_(encoder_map), pattern_(pattern), pattern_length_(pattern_length),
        state_(TrackerState::UNINITIALIZED), current_position_(0),
        current_direction_(SearchDirection::FORWARD), consecutive_failures_(0)
{

    clear_stats();

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "PositionTracker initialized: pattern_length=%d", pattern_length_);
#endif
}

void PositionTracker::initialize_position(uint16_t position, SearchDirection direction)
{
    current_position_ = position;
    current_direction_ = direction;
    state_ = TrackerState::TRACKING;
    consecutive_failures_ = 0;

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Position tracker initialized: position=%d, direction=%s",
            position, (direction == SearchDirection::FORWARD) ? "FORWARD" : "BACKWARD");
#endif
}

ValidationResult PositionTracker::track_position(uint8_t bit, SearchDirection direction, uint16_t &tracked_position)
{
    if(state_ == TrackerState::UNINITIALIZED)
    {
#ifdef LOGE
        LOGE(absolute_encoder_log_level, "Tracker not initialized");
#endif
        return ValidationResult::INVALID;
    }

    stats_.total_updates++;
    current_direction_ = direction;

    // 计算下一个预期位置
    uint16_t expected_position = calculate_next_position(current_position_, direction);

    // 简单验证：检查预期位置的位值是否匹配
    if(pattern_[expected_position] == bit)
    {
        // 匹配成功，更新位置
        current_position_ = expected_position;
        tracked_position = current_position_;
        consecutive_failures_ = 0;
        stats_.successful_tracks++;
        state_ = TrackerState::TRACKING;

#ifdef LOGD
        LOGD(absolute_encoder_log_level, "Position updated to %u, bit=%d", current_position_, bit);
#endif
        return ValidationResult::VALID;
    }
    else
    {
        // 不匹配，记录失败
        consecutive_failures_++;
        stats_.validation_failures++;
        tracked_position = current_position_; // 保持当前位置

#ifdef LOGD
        LOGD(absolute_encoder_log_level, "Position validation failed: expected bit %d at position %u, got %d",
                pattern_[expected_position], expected_position, bit);
#endif

        // 检查是否需要恢复
        if(consecutive_failures_ >= MAX_CONSECUTIVE_FAILURES)
        {
            state_ = TrackerState::LOST;
#ifdef LOGW
            LOGW(absolute_encoder_log_level, "Position tracking lost after %d consecutive failures", consecutive_failures_);
#endif

            // 尝试简单恢复：检查当前位置是否匹配
            if(pattern_[current_position_] == bit)
            {
                // 位置没变，可能是运动停止了
                consecutive_failures_ = 0;
                state_ = TrackerState::TRACKING;
                stats_.position_corrections++;
                return ValidationResult::VALID;
            }

            return ValidationResult::INVALID;
        }
        else
        {
            // 容错范围内，返回不确定
            return ValidationResult::UNCERTAIN;
        }
    }
}

void PositionTracker::reset()
{
    state_ = TrackerState::UNINITIALIZED;
    current_position_ = 0;
    current_direction_ = SearchDirection::FORWARD;
    consecutive_failures_ = 0;

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Position tracker reset");
#endif
}

void PositionTracker::clear_stats()
{
    stats_.total_updates = 0;
    stats_.successful_tracks = 0;
    stats_.validation_failures = 0;
    stats_.direction_changes = 0;
    stats_.recovery_attempts = 0;
    stats_.position_corrections = 0;
}

uint16_t PositionTracker::calculate_next_position(uint16_t current_pos, SearchDirection direction) const
{
    if(direction == SearchDirection::FORWARD)
    {
        return (current_pos + 1) % pattern_length_;
    }
    else
    {
        // 反向计算，注意处理下溢
        return (current_pos == 0) ? (pattern_length_ - 1) : (current_pos - 1);
    }
}

uint16_t PositionTracker::calculate_previous_position(uint16_t current_pos, SearchDirection direction) const
{
    if(direction == SearchDirection::FORWARD)
    {
        // 正向移动时，前一个位置是向后一位
        return (current_pos == 0) ? (pattern_length_ - 1) : (current_pos - 1);
    }
    else
    {
        // 反向移动时，前一个位置是向前一位
        return (current_pos + 1) % pattern_length_;
    }
}

} // namespace AbsoluteEncoder