#include "absolute_encoder.h"
#include "input_preprocessor.hpp"
#include <algorithm>

namespace AbsoluteEncoder
{

InputPreprocessor::InputPreprocessor(const ConversionConfig &conversion_config, const FilterConfig &filter_config)
    : conversion_config_(conversion_config), filter_config_(filter_config),
        accumulated_steps_(0), last_direction_(StepDirection::FORWARD),
        debounce_counter_(0), consecutive_same_signals_(0), consecutive_units_same_direction_(0),
        last_unit_direction_(StepDirection::FORWARD), is_calibrated_(false),
        calibration_in_progress_(false), calibration_step_count_(0)
{

    clear_stats();

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "InputPreprocessor initialized with motor_steps_per_unit=%d, step_tolerance_ratio=%.3f, debounce_samples=%d",
            conversion_config_.motor_steps_per_unit, conversion_config_.step_tolerance_ratio,
            filter_config_.debounce_samples);
#endif
}

InputPreprocessor::ProcessResult InputPreprocessor::process_single_step(StepDirection direction, int32_t &unit_position_change)
{
    stats_.total_steps++;
    unit_position_change = 0;

    ProcessResult result = ProcessResult::VALID;

    // 0. 如果正在校准，执行校准流程
    if(calibration_in_progress_)
    {
        return perform_calibration(direction);
    }

    // 如果未校准且启用了自动对齐，启动校准
    if(!is_calibrated_ && conversion_config_.enable_auto_alignment)
    {
        start_calibration();
        return ProcessResult::CALIBRATING;
    }

    // 1. 检查连续相同信号
    if(filter_config_.enable_pattern_validation && check_consecutive_signals(direction))
    {
        result = ProcessResult::PATTERN_ERROR;
        stats_.pattern_errors++;

#ifdef LOGW
        LOGW(absolute_encoder_log_level, "Pattern error detected: consecutive_same_signals=%d", consecutive_same_signals_);
#endif
    }

    // 2. 防抖滤波
    if(!apply_debounce_filter(direction))
    {
        result = ProcessResult::DEBOUNCED;
        stats_.debounce_events++;

#ifdef LOGV
        LOGV(absolute_encoder_log_level, "Step debounced: direction=%d", (int)direction);
#endif

        return result;
    }

    // 3. 方向滤波（如果启用）
    StepDirection filtered_direction = direction;
    if(filter_config_.enable_direction_filter)
    {
        filtered_direction = apply_direction_filter(direction);
    }

    // 4. 累积步数并应用容差调整
    uint16_t unit_threshold = apply_tolerance_adjustment();

    if(filtered_direction == StepDirection::FORWARD)
    {
        accumulated_steps_++;
    }
    else
    {
        if(accumulated_steps_ > 0)
        {
            accumulated_steps_--;
        }
        else
        {
            // 处理反向超出的情况 - 从另一个单位后退
            accumulated_steps_ = unit_threshold - 1;
            unit_position_change = -1;
            result = ProcessResult::UNIT_RETREATED;
            stats_.unit_retreats++;
            update_average_steps(unit_threshold);

            // 检测长距离相同方向运动（后退）
            if(last_unit_direction_ == StepDirection::BACKWARD)
            {
                consecutive_units_same_direction_++;
            }
            else
            {
                consecutive_units_same_direction_ = 1;
            }
            last_unit_direction_ = StepDirection::BACKWARD;

            // 更新最大连续单位数记录
            if(consecutive_units_same_direction_ > stats_.max_consecutive_units)
            {
                stats_.max_consecutive_units = consecutive_units_same_direction_;
            }
        }
    }

    // 5. 检查是否达到单位长度（使用动态阈值）
    if(accumulated_steps_ >= unit_threshold)
    {
        uint16_t actual_steps = accumulated_steps_;
        accumulated_steps_ = 0;
        unit_position_change = 1;

        if(result == ProcessResult::VALID || result == ProcessResult::PATTERN_ERROR)
        {
            result = ProcessResult::UNIT_ADVANCED;
        }

        stats_.unit_advances++;
        update_average_steps(actual_steps);

        // 检测长距离相同方向运动
        if(last_unit_direction_ == StepDirection::FORWARD)
        {
            consecutive_units_same_direction_++;
        }
        else
        {
            consecutive_units_same_direction_ = 1;
        }
        last_unit_direction_ = StepDirection::FORWARD;

        // 更新最大连续单位数记录
        if(consecutive_units_same_direction_ > stats_.max_consecutive_units)
        {
            stats_.max_consecutive_units = consecutive_units_same_direction_;
        }

        // 检查是否需要容差调整
        if(abs((int)actual_steps - (int)conversion_config_.motor_steps_per_unit) >
                conversion_config_.max_step_deviation)
        {
            result = ProcessResult::TOLERANCE_ADJUSTED;
            stats_.tolerance_adjustments++;

#ifdef LOGD
            LOGD(absolute_encoder_log_level, "Tolerance adjustment: actual_steps=%d, expected=%d, deviation=%d",
                    actual_steps, conversion_config_.motor_steps_per_unit,
                    abs((int)actual_steps - (int)conversion_config_.motor_steps_per_unit));
#endif
        }
    }

    // 更新状态
    last_direction_ = filtered_direction;

    // 更新统计
    update_stats(result, filtered_direction, unit_position_change);

#ifdef LOGV
    LOGV(absolute_encoder_log_level, "Step processed: direction=%d, accumulated=%d, unit_change=%d, result=%d, threshold=%d",
            (int)filtered_direction, accumulated_steps_, unit_position_change, (int)result, unit_threshold);
#endif

    return result;
}

bool InputPreprocessor::apply_debounce_filter(StepDirection direction)
{
    if(direction == last_direction_)
    {
        // 方向相同，重置防抖计数器
        debounce_counter_ = 0;
        return true;
    }
    else
    {
        // 方向不同，增加防抖计数器
        debounce_counter_++;

        // 检查是否达到防抖阈值
        if(debounce_counter_ >= filter_config_.debounce_samples)
        {
            debounce_counter_ = 0;
            return true;  // 通过防抖
        }
        else
        {
            return false; // 被防抖过滤
        }
    }
}

InputPreprocessor::StepDirection InputPreprocessor::apply_direction_filter(StepDirection direction)
{
    direction_history_.push_back(direction);

    // 保持历史记录在指定大小内
    if(direction_history_.size() > filter_config_.direction_filter_window)
    {
        direction_history_.pop_front();
    }

    // 如果样本不够，返回当前方向
    if(direction_history_.size() < filter_config_.direction_filter_window)
    {
        return direction;
    }

    // 多数决策
    uint8_t forward_count = 0;
    for(StepDirection dir : direction_history_)
    {
        if(dir == StepDirection::FORWARD)
        {
            forward_count++;
        }
    }

    return (forward_count > direction_history_.size() / 2) ?
            StepDirection::FORWARD : StepDirection::BACKWARD;
}

void InputPreprocessor::update_stats(ProcessResult result, StepDirection direction, int32_t unit_change)
{
    stats_.accumulated_steps = accumulated_steps_;
    stats_.consecutive_same_signals = consecutive_same_signals_;
    stats_.consecutive_units_same_direction = consecutive_units_same_direction_;

    // 这里可以添加更多统计信息的更新
    (void)result;      // 避免未使用变量警告
    (void)direction;
    (void)unit_change;
}

void InputPreprocessor::reset()
{
    accumulated_steps_ = 0;
    last_direction_ = StepDirection::FORWARD;
    debounce_counter_ = 0;
    consecutive_same_signals_ = 0;
    consecutive_units_same_direction_ = 0;
    last_unit_direction_ = StepDirection::FORWARD;
    direction_history_.clear();
    unit_steps_history_.clear();

    // 重置校准状态
    is_calibrated_ = false;
    calibration_in_progress_ = false;
    calibration_step_count_ = 0;
    calibration_samples_.clear();

    clear_stats();

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "InputPreprocessor reset");
#endif
}

void InputPreprocessor::update_conversion_config(const ConversionConfig &config)
{
    conversion_config_ = config;
    // 重置累积步数和历史，因为单位长度可能已改变
    accumulated_steps_ = 0;
    unit_steps_history_.clear();
    stats_.average_steps_per_unit = 0.0f;
#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Conversion config updated");
#endif
}

void InputPreprocessor::update_filter_config(const FilterConfig &config)
{
    filter_config_ = config;
    debounce_counter_ = 0;
    consecutive_same_signals_ = 0;
    direction_history_.clear();
#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Filter config updated");
#endif
}

void InputPreprocessor::clear_stats()
{
    stats_.total_steps = 0;
    stats_.unit_advances = 0;
    stats_.unit_retreats = 0;
    stats_.debounce_events = 0;
    stats_.tolerance_adjustments = 0;
    stats_.pattern_errors = 0;
    stats_.accumulated_steps = 0;
    stats_.average_steps_per_unit = 0.0f;
    stats_.consecutive_same_signals = 0;
    stats_.consecutive_units_same_direction = 0;
    stats_.max_consecutive_units = 0;
    stats_.is_calibrated = is_calibrated_;
    stats_.calibration_progress = calibration_step_count_;
}

bool InputPreprocessor::check_consecutive_signals(StepDirection direction)
{
    if(direction == last_direction_)
    {
        consecutive_same_signals_++;
    }
    else
    {
        consecutive_same_signals_ = 1;
    }

    // 检查是否超过阈值 - 这应该是步数级别的阈值，不是单位级别
    // 对于长距离运动，连续相同方向的步数可能很多，这是正常的
    return consecutive_same_signals_ > filter_config_.consistency_threshold;
}

uint16_t InputPreprocessor::apply_tolerance_adjustment()
{
    uint16_t base_threshold = conversion_config_.motor_steps_per_unit;

    if(!conversion_config_.enable_adaptive_correction)
    {
        return base_threshold;
    }

    // 基于历史平均值进行自适应调整
    float avg_steps = get_average_steps_per_unit();
    if(avg_steps > 0.0f && unit_steps_history_.size() >= 2)     // 降低要求
    {
        float tolerance = base_threshold * conversion_config_.step_tolerance_ratio;
        float adjusted_threshold = avg_steps;

        // 限制调整范围
        if(adjusted_threshold < base_threshold - tolerance)
        {
            adjusted_threshold = base_threshold - tolerance;
            stats_.tolerance_adjustments++;  // 记录调整
        }
        else if(adjusted_threshold > base_threshold + tolerance)
        {
            adjusted_threshold = base_threshold + tolerance;
            stats_.tolerance_adjustments++;  // 记录调整
        }

        // 如果调整幅度足够大，记录为容差调整
        if(abs(adjusted_threshold - base_threshold) > 2.0f)
        {
            stats_.tolerance_adjustments++;
        }

        return (uint16_t)adjusted_threshold;
    }

    return base_threshold;
}

void InputPreprocessor::update_average_steps(uint16_t steps_taken)
{
    unit_steps_history_.push_back(steps_taken);

    // 保持历史记录在合理大小内
    if(unit_steps_history_.size() > 10)
    {
        unit_steps_history_.pop_front();
    }

    // 计算平均值
    if(!unit_steps_history_.empty())
    {
        uint32_t sum = 0;
        for(uint16_t steps : unit_steps_history_)
        {
            sum += steps;
        }
        stats_.average_steps_per_unit = (float)sum / unit_steps_history_.size();
    }
}

float InputPreprocessor::get_average_steps_per_unit() const
{
    return stats_.average_steps_per_unit;
}

void InputPreprocessor::correct_accumulated_steps(int16_t correction)
{
    int32_t new_steps = (int32_t)accumulated_steps_ + correction;

    // 确保步数在合理范围内
    if(new_steps < 0)
    {
        accumulated_steps_ = 0;
    }
    else if(new_steps > conversion_config_.motor_steps_per_unit * 2)
    {
        accumulated_steps_ = conversion_config_.motor_steps_per_unit * 2;
    }
    else
    {
        accumulated_steps_ = (uint16_t)new_steps;
    }

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Manual step correction: correction=%d, new_accumulated_steps=%d",
            correction, accumulated_steps_);
#endif
}

void InputPreprocessor::start_calibration()
{
    if(calibration_in_progress_)
    {
        return;
    }

    calibration_in_progress_ = true;
    calibration_step_count_ = 0;
    calibration_samples_.clear();
    accumulated_steps_ = 0;

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Starting auto-calibration process");
#endif
}

void InputPreprocessor::set_calibrated(bool calibrated, uint16_t reference_position)
{
    is_calibrated_ = calibrated;
    calibration_in_progress_ = false;

    if(calibrated)
    {
        accumulated_steps_ = reference_position;
#ifdef LOGD
        LOGD(absolute_encoder_log_level, "Calibration set manually: reference_position=%d", reference_position);
#endif
    }
    else
    {
        accumulated_steps_ = 0;
        unit_steps_history_.clear();
        stats_.average_steps_per_unit = 0.0f;
    }
}

InputPreprocessor::ProcessResult InputPreprocessor::perform_calibration(StepDirection direction)
{
    calibration_step_count_++;

    // 简单的校准逻辑：收集一定数量的单位步数样本
    if(direction == StepDirection::FORWARD)
    {
        accumulated_steps_++;
    }
    else
    {
        if(accumulated_steps_ > 0)
        {
            accumulated_steps_--;
        }
    }

    // 检测单位边界（假设在预期步数附近）
    uint16_t expected_steps = conversion_config_.motor_steps_per_unit;
    if(accumulated_steps_ >= expected_steps - conversion_config_.max_step_deviation &&
            accumulated_steps_ <= expected_steps + conversion_config_.max_step_deviation)
    {

        // 可能的单位边界，记录样本
        calibration_samples_.push_back(accumulated_steps_);
        accumulated_steps_ = 0;

#ifdef LOGV
        LOGV(absolute_encoder_log_level, "Calibration sample collected: %d, total samples: %zu",
                calibration_samples_.back(), calibration_samples_.size());
#endif
    }

    // 检查是否收集到足够的样本
    if(calibration_samples_.size() >= conversion_config_.alignment_sample_count)
    {
        if(finalize_calibration())
        {
            return ProcessResult::VALID;  // 校准完成，返回正常状态
        }
    }

    // 防止校准时间过长
    if(calibration_step_count_ > expected_steps * conversion_config_.alignment_sample_count * 3)
    {
#ifdef LOGW
        LOGW(absolute_encoder_log_level, "Calibration timeout, using default configuration");
#endif
        is_calibrated_ = true;
        calibration_in_progress_ = false;
        accumulated_steps_ = 0;
        return ProcessResult::VALID;  // 超时后返回正常状态
    }

    return ProcessResult::CALIBRATING;
}

bool InputPreprocessor::finalize_calibration()
{
    if(calibration_samples_.empty())
    {
        return false;
    }

    // 计算样本的平均值和标准差
    uint32_t sum = 0;
    for(uint16_t sample : calibration_samples_)
    {
        sum += sample;
    }
    float average = (float)sum / calibration_samples_.size();

    // 检查样本一致性
    float max_deviation = 0.0f;
    for(uint16_t sample : calibration_samples_)
    {
        float deviation = abs((float)sample - average);
        if(deviation > max_deviation)
        {
            max_deviation = deviation;
        }
    }

    // 如果样本一致性足够好，完成校准
    float tolerance_threshold = conversion_config_.motor_steps_per_unit * conversion_config_.alignment_tolerance;
    if(max_deviation <= tolerance_threshold)
    {
        // 更新配置
        conversion_config_.motor_steps_per_unit = (uint16_t)average;
        stats_.average_steps_per_unit = average;

        // 设置校准完成
        is_calibrated_ = true;
        calibration_in_progress_ = false;
        accumulated_steps_ = 0;

#ifdef LOGD
        LOGD(absolute_encoder_log_level, "Calibration completed: average_steps_per_unit=%.2f, max_deviation=%.2f",
                average, max_deviation);
#endif
        return true;
    }
    else
    {
#ifdef LOGW
        LOGW(absolute_encoder_log_level, "Calibration failed: max_deviation=%.2f > threshold=%.2f",
                max_deviation, tolerance_threshold);
#endif
        return false;
    }
}

bool InputPreprocessor::detect_alignment_boundary()
{
    // 基于步数模式检测对齐边界
    if(unit_steps_history_.size() < 3)
    {
        return false;
    }

    // 检查最近几个单位的步数是否稳定
    uint16_t recent_avg = 0;
    for(size_t i = unit_steps_history_.size() - 3; i < unit_steps_history_.size(); ++i)
    {
        recent_avg += unit_steps_history_[i];
    }
    recent_avg /= 3;

    // 如果步数接近标准值，认为已对齐
    uint16_t deviation = abs((int)recent_avg - (int)conversion_config_.motor_steps_per_unit);
    return deviation <= conversion_config_.max_step_deviation;
}

} // namespace AbsoluteEncoder