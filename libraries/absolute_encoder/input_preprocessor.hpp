#ifndef INPUT_PREPROCESSOR_HPP
#define INPUT_PREPROCESSOR_HPP

#include <cstdint>
#include <deque>

namespace AbsoluteEncoder
{

/**
 * 输入信号预处理器
 * 在电机控制中断中处理每一步的信号：
 * - 电机步进到编码器单位的累积换算
 * - 防抖滤波（消除机械抖动）
 * - 累积误差管理
 */
class InputPreprocessor
{
public:
    /**
     * 换算配置参数
     */
    struct ConversionConfig
    {
        uint16_t motor_steps_per_unit;      // 电机多少步对应编码器前进1个单位
        float step_tolerance_ratio;         // 步数容差比例 (0.0-1.0)
        uint16_t max_step_deviation;        // 最大允许步数偏差
        bool enable_adaptive_correction;    // 是否启用自适应修正
        bool enable_auto_alignment;         // 是否启用自动对齐
        uint16_t alignment_sample_count;    // 对齐采样数量
        float alignment_tolerance;          // 对齐容差
    };

    /**
     * 滤波配置参数
     */
    struct FilterConfig
    {
        uint8_t debounce_samples;           // 防抖采样数
        bool enable_direction_filter;       // 是否启用方向滤波
        uint8_t direction_filter_window;    // 方向滤波窗口大小
        uint8_t consistency_threshold;      // 连续相同信号阈值
        bool enable_pattern_validation;     // 是否启用模式验证
        bool enable_auto_alignment;         // 是否启用自动对齐
        uint16_t calibration_steps;         // 校准所需步数
    };

    /**
     * 处理结果状态
     */
    enum class ProcessResult
    {
        VALID,              // 正常处理
        DEBOUNCED,          // 防抖过滤
        UNIT_ADVANCED,      // 编码器单位前进
        UNIT_RETREATED,     // 编码器单位后退
        TOLERANCE_ADJUSTED, // 容差调整
        PATTERN_ERROR,      // 模式错误
        ALIGNMENT_DETECTED, // 检测到对齐
        CALIBRATING         // 正在校准中
    };

    /**
     * 步进方向
     */
    enum class StepDirection
    {
        FORWARD = 1,        // 前进
        BACKWARD = -1       // 后退
    };

    /**
     * 处理统计信息
     */
    struct ProcessingStats
    {
        uint32_t total_steps;               // 总步数
        uint32_t unit_advances;             // 编码器单位前进次数
        uint32_t unit_retreats;             // 编码器单位后退次数
        uint32_t debounce_events;           // 防抖事件次数
        uint32_t tolerance_adjustments;     // 容差调整次数
        uint32_t pattern_errors;            // 模式错误次数
        uint16_t accumulated_steps;         // 当前累积步数
        float average_steps_per_unit;       // 平均每单位步数
        uint8_t consecutive_same_signals;   // 连续相同信号计数
        uint32_t consecutive_units_same_direction; // 连续相同方向的单位数
        uint32_t max_consecutive_units;     // 最大连续单位数记录
        bool is_calibrated;                 // 是否已校准
        uint32_t calibration_progress;      // 校准进度
    };

public:
    /**
     * 构造函数
     * @param conversion_config 换算配置
     * @param filter_config 滤波配置
     */
    InputPreprocessor(const ConversionConfig &conversion_config, const FilterConfig &filter_config);

    /**
     * 处理单步电机信号
     * @param direction 步进方向
     * @param unit_position_change 输出：编码器位置变化（单位长度）
     * @return 处理结果状态
     */
    ProcessResult process_single_step(StepDirection direction, int32_t &unit_position_change);

    /**
     * 获取当前累积步数
     */
    uint16_t get_accumulated_steps() const
    {
        return accumulated_steps_;
    }

    /**
     * 获取平均步数（用于自适应调整）
     */
    float get_average_steps_per_unit() const;

    /**
     * 手动校正累积步数
     * @param correction 校正值
     */
    void correct_accumulated_steps(int16_t correction);

    /**
     * 检查连续相同信号状态
     */
    uint8_t get_consecutive_same_signals() const
    {
        return consecutive_same_signals_;
    }

    /**
     * 启动自动校准
     */
    void start_calibration();

    /**
     * 检查是否已校准
     */
    bool is_calibrated() const
    {
        return is_calibrated_;
    }

    /**
     * 强制设置校准状态
     * @param calibrated 校准状态
     * @param reference_position 参考位置步数
     */
    void set_calibrated(bool calibrated, uint16_t reference_position = 0);

    /**
     * 重置预处理器状态
     */
    void reset();

    /**
     * 更新配置
     */
    void update_conversion_config(const ConversionConfig &config);
    void update_filter_config(const FilterConfig &config);

    /**
     * 获取统计信息
     */
    const ProcessingStats &get_stats() const
    {
        return stats_;
    }

    /**
     * 清零统计信息
     */
    void clear_stats();

private:
    // 配置参数
    ConversionConfig conversion_config_;
    FilterConfig filter_config_;

    // 内部状态
    uint16_t accumulated_steps_;        // 累积步数
    StepDirection last_direction_;      // 上次方向
    uint8_t debounce_counter_;         // 防抖计数器
    uint8_t consecutive_same_signals_; // 连续相同信号计数
    uint32_t consecutive_units_same_direction_; // 连续相同方向的单位数
    StepDirection last_unit_direction_; // 上次单位移动方向
    std::deque<StepDirection> direction_history_; // 方向历史
    std::deque<uint16_t> unit_steps_history_;     // 每单位步数历史

    // 校准相关状态
    bool is_calibrated_;               // 是否已校准
    bool calibration_in_progress_;     // 校准是否进行中
    uint32_t calibration_step_count_;  // 校准步数计数
    std::deque<uint16_t> calibration_samples_; // 校准样本

    // 统计信息
    ProcessingStats stats_;

    /**
     * 防抖滤波
     * @param direction 输入方向
     * @return 是否通过防抖
     */
    bool apply_debounce_filter(StepDirection direction);

    /**
     * 方向滤波
     * @param direction 输入方向
     * @return 过滤后的方向
     */
    StepDirection apply_direction_filter(StepDirection direction);

    /**
     * 检查连续相同信号
     * @param direction 当前方向
     * @return 是否检测到异常
     */
    bool check_consecutive_signals(StepDirection direction);

    /**
     * 应用容差调整
     * @return 调整后的步数阈值
     */
    uint16_t apply_tolerance_adjustment();

    /**
     * 更新平均步数统计
     * @param steps_taken 实际步数
     */
    void update_average_steps(uint16_t steps_taken);

    /**
     * 执行校准流程
     * @param direction 当前方向
     * @return 校准结果
     */
    ProcessResult perform_calibration(StepDirection direction);

    /**
     * 分析校准样本并完成校准
     * @return 是否校准成功
     */
    bool finalize_calibration();

    /**
     * 检测对齐边界
     * @return 是否检测到边界
     */
    bool detect_alignment_boundary();

    /**
     * 更新统计信息
     */
    void update_stats(ProcessResult result, StepDirection direction, int32_t unit_change);
};

} // namespace AbsoluteEncoder

#endif // INPUT_PREPROCESSOR_HPP