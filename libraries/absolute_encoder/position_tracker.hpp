#ifndef POSITION_TRACKER_HPP
#define POSITION_TRACKER_HPP

#include "encoder_map.hpp"
#include <cstdint>

namespace AbsoluteEncoder
{

/**
 * 位置跟踪器状态
 */
enum class TrackerState
{
    UNINITIALIZED,  // 未初始化
    TRACKING,       // 正常跟踪
    LOST,          // 跟踪丢失
    RECOVERING     // 恢复中
};

/**
 * 跟踪验证结果
 */
enum class ValidationResult
{
    VALID,         // 位置有效
    INVALID,       // 位置无效
    UNCERTAIN      // 不确定（需要更多数据）
};

/**
 * 位置跟踪器
 * 负责在已知位置基础上的实时位置跟踪
 */
class PositionTracker
{
public:
    /**
     * 构造函数
     * @param encoder_map 编码器映射对象引用
     * @param pattern 码盘图案数据
     * @param pattern_length 码盘图案长度
     */
    PositionTracker(EncoderMap &encoder_map, const uint8_t *pattern,
                    uint16_t pattern_length);

    /**
     * 设置初始位置，进入跟踪模式
     * @param position 初始位置
     * @param direction 初始运动方向
     */
    void initialize_position(uint16_t position, SearchDirection direction);

    /**
     * 添加新的输入位并更新跟踪位置
     * @param bit 新输入的位值
     * @param direction 当前运动方向
     * @param tracked_position 输出参数：跟踪到的位置
     * @return 跟踪结果
     */
    ValidationResult track_position(uint8_t bit, SearchDirection direction, uint16_t &tracked_position);

    /**
     * 重置跟踪器状态
     */
    void reset();

    /**
     * 获取当前跟踪状态
     */
    TrackerState get_state() const
    {
        return state_;
    }

    /**
     * 获取当前位置
     */
    uint16_t get_current_position() const
    {
        return current_position_;
    }

    /**
     * 获取当前方向
     */
    SearchDirection get_current_direction() const
    {
        return current_direction_;
    }

    /**
     * 跟踪统计信息
     */
    struct TrackingStats
    {
        uint32_t total_updates;        // 总更新次数
        uint32_t successful_tracks;    // 成功跟踪次数
        uint32_t validation_failures;  // 验证失败次数
        uint32_t direction_changes;    // 方向改变次数
        uint32_t recovery_attempts;    // 恢复尝试次数
        uint32_t position_corrections; // 位置修正次数
    };

    /**
     * 获取跟踪统计信息
     */
    const TrackingStats &get_stats() const
    {
        return stats_;
    }

    /**
     * 清零统计信息
     */
    void clear_stats();

    /**
     * 检查跟踪器是否就绪
     */
    bool is_ready() const
    {
        return state_ != TrackerState::UNINITIALIZED;
    }

private:
    EncoderMap &encoder_map_;           // 编码器映射引用
    const uint8_t *pattern_;            // 码盘图案
    uint16_t pattern_length_;           // 码盘图案长度

    // 跟踪状态
    TrackerState state_;                // 当前状态
    uint16_t current_position_;         // 当前位置
    SearchDirection current_direction_; // 当前方向

    // 验证相关
    uint8_t consecutive_failures_;      // 连续失败次数
    static const uint8_t MAX_CONSECUTIVE_FAILURES = 3; // 最大连续失败次数

    // 统计信息
    TrackingStats stats_;

    /**
     * 计算下一个预期位置
     * @param current_pos 当前位置
     * @param direction 运动方向
     * @return 下一个位置
     */
    uint16_t calculate_next_position(uint16_t current_pos, SearchDirection direction) const;

    /**
     * 计算前一个位置
     * @param current_pos 当前位置
     * @param direction 移动方向
     * @return 前一个位置
     */
    uint16_t calculate_previous_position(uint16_t current_pos, SearchDirection direction) const;
};

} // namespace AbsoluteEncoder

#endif // POSITION_TRACKER_HPP