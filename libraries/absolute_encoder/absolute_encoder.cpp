#include "absolute_encoder.h"
#include "encoder_map.hpp"
#include "smart_search_manager.hpp"
#include "position_tracker.hpp"
#include "input_preprocessor.hpp"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <new>
#include <vector>

using namespace AbsoluteEncoder;

int absolute_encoder_log_level = 0;

/**
 * 编码器图案句柄实现
 */
struct encoder_map_handle_s
{
    EncoderMap *encoder_map;
};

/**
 * 编码器句柄实现
 */
struct encoder_handle_s
{
    // 共享的编码器图案
    encoder_map_handle_t map_handle;

    // 各功能模块
    InputPreprocessor *input_processor;
    SmartSearchManager *search_manager;
    PositionTracker *position_tracker;

    // 配置和状态
    encoder_config_t config;
    encoder_state_t current_state;
    encoder_position_t current_position;
    encoder_stats_t stats;

    // 运行时状态
    clock_t start_time;
    uint32_t last_signal_timestamp;
    bool debug_enabled;
};

/**
 * 内部辅助函数声明
 */
static encoder_result_t handle_search_mode(encoder_handle_t handle, uint8_t signal_bit);
static encoder_result_t handle_tracking_mode(encoder_handle_t handle, uint8_t signal_bit);
static void update_runtime_stats(encoder_handle_t handle);
static void transition_to_tracking_mode(encoder_handle_t handle, uint32_t found_position);
static void transition_to_search_mode(encoder_handle_t handle);

// === 日志等级设置 ===

void absolute_encoder_set_log_level(int level)
{
    absolute_encoder_log_level = level;
}

// === 编码器图案对象管理 ===

encoder_map_handle_t create_encoder_map(const uint8_t *pattern_data, uint32_t pattern_length, uint8_t max_search_depth)
{
    if(!pattern_data || pattern_length == 0 || max_search_depth == 0)
    {
        return NULL;
    }

    encoder_map_handle_t map_handle = (encoder_map_handle_t)malloc(sizeof(struct encoder_map_handle_s));
    if(!map_handle)
    {
        return NULL;
    }

    // 将C数组转换为std::vector
    std::vector<uint8_t> pattern(pattern_data, pattern_data + pattern_length);
    map_handle->encoder_map = new(std::nothrow) EncoderMap(pattern_data, pattern_length, max_search_depth);
    if(!map_handle->encoder_map)
    {
        free(map_handle);
        return NULL;
    }

    return map_handle;
}

encoder_map_handle_t create_encoder_map_with_pool(const uint8_t *pattern_data, uint32_t pattern_length, uint8_t max_search_depth, void *node_pool, uint16_t pool_size)
{
    if(!pattern_data || pattern_length == 0 || max_search_depth == 0 || !node_pool || pool_size == 0)
    {
        return NULL;
    }

    encoder_map_handle_t map_handle = (encoder_map_handle_t)malloc(sizeof(struct encoder_map_handle_s));
    if(!map_handle)
    {
        return NULL;
    }

    map_handle->encoder_map = new(std::nothrow) EncoderMap(pattern_data, pattern_length, max_search_depth, (TreeNode *)node_pool, pool_size);
    if(!map_handle->encoder_map)
    {
        free(map_handle);
        return NULL;
    }

    return map_handle;
}

void destroy_encoder_map(encoder_map_handle_t map_handle)
{
    if(map_handle)
    {
        delete map_handle->encoder_map;
        free(map_handle);
    }
}

// === 编码器对象管理 ===

encoder_handle_t create_encoder(encoder_map_handle_t map_handle, const encoder_config_t *config)
{
    if(!map_handle || !config)
    {
        return NULL;
    }

    encoder_handle_t handle = (encoder_handle_t)malloc(sizeof(struct encoder_handle_s));
    if(!handle)
    {
        return NULL;
    }

    // 初始化基本信息
    memset(handle, 0, sizeof(struct encoder_handle_s));
    handle->map_handle = map_handle;
    handle->config = *config;
    handle->current_state = ENCODER_STATE_SEARCHING;
    if(config->sys_tick_get != nullptr)
    {
        handle->start_time = config->sys_tick_get();
    }

    // 创建输入预处理器
    InputPreprocessor::ConversionConfig conv_config;
    conv_config.motor_steps_per_unit = config->motor_steps_per_unit;
    conv_config.step_tolerance_ratio = 0.1f;
    conv_config.max_step_deviation = 5;
    conv_config.enable_adaptive_correction = true;
    conv_config.enable_auto_alignment = true;
    conv_config.alignment_sample_count = 3;
    conv_config.alignment_tolerance = 0.1f;

    InputPreprocessor::FilterConfig filter_config;
    filter_config.debounce_samples = 3;
    filter_config.enable_direction_filter = true;
    filter_config.direction_filter_window = 5;
    filter_config.consistency_threshold = 20;
    filter_config.enable_pattern_validation = true;

    handle->input_processor = new(std::nothrow) InputPreprocessor(conv_config, filter_config);
    if(!handle->input_processor)
    {
        free(handle);
        return NULL;
    }

    // 创建搜索管理器
    handle->search_manager = new(std::nothrow) SmartSearchManager(*map_handle->encoder_map);
    if(!handle->search_manager)
    {
        delete handle->input_processor;
        free(handle);
        return NULL;
    }

    // 创建位置跟踪器
    const uint8_t *pattern_data = map_handle->encoder_map->get_pattern_data();
    uint16_t pattern_length = map_handle->encoder_map->get_pattern_length();
    handle->position_tracker = new(std::nothrow) PositionTracker(*map_handle->encoder_map, pattern_data, pattern_length);
    if(!handle->position_tracker)
    {
        delete handle->input_processor;
        delete handle->search_manager;
        free(handle);
        return NULL;
    }

    return handle;
}

void destroy_encoder(encoder_handle_t handle)
{
    if(handle)
    {
        delete handle->input_processor;
        delete handle->search_manager;
        delete handle->position_tracker;
        free(handle);
    }
}

// === 核心信号处理 ===

encoder_result_t process_encoder_signal(encoder_handle_t handle, uint8_t signal_bit, encoder_position_t *position_info)
{
    if(!handle || !position_info)
    {
        return ENCODER_RESULT_ERROR_INVALID_SIGNAL;
    }

    // 更新统计信息
    handle->stats.total_signals_processed++;
    update_runtime_stats(handle);

    // 根据当前状态处理信号
    encoder_result_t result;
    switch(handle->current_state)
    {
    case ENCODER_STATE_SEARCHING:
        result = handle_search_mode(handle, signal_bit);
        break;

    case ENCODER_STATE_TRACKING:
        result = handle_tracking_mode(handle, signal_bit);
        break;

    case ENCODER_STATE_ERROR:
        // 错误状态下尝试重新搜索
        transition_to_search_mode(handle);
        result = ENCODER_RESULT_SEARCHING;
        break;

    default:
        handle->current_state = ENCODER_STATE_ERROR;
        result = ENCODER_RESULT_ERROR_INVALID_SIGNAL;
        break;
    }

    // 输出当前位置信息
    *position_info = handle->current_position;

    return result;
}

encoder_result_t process_motor_steps(encoder_handle_t handle, uint16_t steps, int8_t direction)
{
    if(!handle)
    {
        return ENCODER_RESULT_ERROR_INVALID_SIGNAL;
    }

    // 处理电机步进信号
    for(uint16_t i = 0; i < steps; ++i)
    {
        int32_t unit_change;
        InputPreprocessor::StepDirection step_dir = (direction > 0) ?
            InputPreprocessor::StepDirection::FORWARD :
            InputPreprocessor::StepDirection::BACKWARD;

        auto result = handle->input_processor->process_single_step(step_dir, unit_change);

        // 如果有单位变化，通知位置跟踪器（如果在跟踪模式）
        if(unit_change != 0 && handle->current_state == ENCODER_STATE_TRACKING)
        {
            handle->current_position.relative_position_change += unit_change;

            // 可以在这里添加运动预测逻辑
#ifdef LOGD
            LOGD(absolute_encoder_log_level, "Motor feedback: unit_change=%d, total_change=%d\n",
                    unit_change, handle->current_position.relative_position_change);
#endif
        }
    }

    return ENCODER_RESULT_OK;
}

// === 状态查询接口 ===

bool get_current_position(encoder_handle_t handle, encoder_position_t *position_info)
{
    if(!handle || !position_info)
    {
        return false;
    }

    *position_info = handle->current_position;
    return true;
}

bool get_encoder_stats(encoder_handle_t handle, encoder_stats_t *stats)
{
    if(!handle || !stats)
    {
        return false;
    }

    update_runtime_stats(handle);
    *stats = handle->stats;
    return true;
}

bool reset_encoder(encoder_handle_t handle)
{
    if(!handle)
    {
        return false;
    }

    // 重置所有模块
    handle->input_processor->reset();
    handle->search_manager->reset();
    handle->position_tracker->reset();

    // 重置状态
    handle->current_state = ENCODER_STATE_SEARCHING;
    memset(&handle->current_position, 0, sizeof(encoder_position_t));
    handle->current_position.state = ENCODER_STATE_SEARCHING;
    memset(&handle->stats, 0, sizeof(encoder_stats_t));
    if(handle->config.sys_tick_get != nullptr)
    {
        handle->start_time = handle->config.sys_tick_get();
    }

    return true;
}

bool force_search_mode(encoder_handle_t handle)
{
    if(!handle)
    {
        return false;
    }

    transition_to_search_mode(handle);
    return true;
}

// === 内部状态处理函数 ===

static encoder_result_t handle_search_mode(encoder_handle_t handle, uint8_t signal_bit)
{
    handle->stats.search_attempts++;

    // 检查搜索超时
    if(handle->config.sys_tick_get != nullptr)
    {
        uint64_t current_time = handle->config.sys_tick_get();
        uint64_t elapsed_seconds = current_time - handle->start_time;
        if(handle->config.search_timeout_seconds > 0
        && elapsed_seconds > handle->config.search_timeout_seconds)
        {
            handle->current_state = ENCODER_STATE_ERROR;
            handle->stats.error_count++;
            return ENCODER_RESULT_ERROR_TIMEOUT;
        }
    }

    // 使用智能搜索管理器进行搜索
    uint16_t found_position;
    SearchResult search_result = handle->search_manager->add_bit_and_search(
            signal_bit, SearchDirection::FORWARD, found_position);

    switch(search_result)
    {
    case SearchResult::FOUND:
        // 找到位置，切换到跟踪模式
        transition_to_tracking_mode(handle, found_position);
        handle->current_position.absolute_position = found_position;
        handle->current_position.confidence_level = 100;
        return ENCODER_RESULT_POSITION_FOUND;

    case SearchResult::PARTIAL:
        // 部分匹配，继续搜索
        handle->current_position.confidence_level = 60;
        return ENCODER_RESULT_SEARCHING;

    case SearchResult::NOT_FOUND:
        // 未找到匹配，继续搜索
        handle->current_position.confidence_level = 20;
        return ENCODER_RESULT_SEARCHING;
    }

    return ENCODER_RESULT_SEARCHING;
}

static encoder_result_t handle_tracking_mode(encoder_handle_t handle, uint8_t signal_bit)
{
    handle->stats.tracking_updates++;

    // 使用位置跟踪器进行跟踪
    uint16_t tracked_position;
    ValidationResult validation_result = handle->position_tracker->track_position(
            signal_bit, SearchDirection::FORWARD, tracked_position);

    switch(validation_result)
    {
    case ValidationResult::VALID:
        // 跟踪成功
        handle->current_position.absolute_position = tracked_position;
        handle->current_position.relative_position_change = 1; // 前进了一个位置
        handle->current_position.confidence_level = 95;
        return ENCODER_RESULT_TRACKING_UPDATED;

    case ValidationResult::UNCERTAIN:
        // 跟踪不确定，保持当前位置
        handle->current_position.confidence_level = 70;
        return ENCODER_RESULT_OK;

    case ValidationResult::INVALID:
        // 跟踪失败，切换到搜索模式
        transition_to_search_mode(handle);
        handle->stats.error_count++;
        return ENCODER_RESULT_ERROR_LOST_TRACKING;
    }

    return ENCODER_RESULT_OK;
}

static void update_runtime_stats(encoder_handle_t handle)
{
    uint64_t current_time = handle->config.sys_tick_get();
    handle->stats.runtime_ticks = current_time - handle->start_time;
}

static void transition_to_tracking_mode(encoder_handle_t handle, uint32_t found_position)
{
    handle->current_state = ENCODER_STATE_TRACKING;
    handle->current_position.state = ENCODER_STATE_TRACKING;
    handle->current_position.absolute_position = found_position;
    handle->current_position.relative_position_change = 0;
    handle->current_position.confidence_level = 100;

    // 初始化位置跟踪器
    handle->position_tracker->initialize_position(found_position, SearchDirection::FORWARD);

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Encoder: Switched to tracking mode at position %u\n", found_position);
#endif
}

static void transition_to_search_mode(encoder_handle_t handle)
{
    handle->current_state = ENCODER_STATE_SEARCHING;
    handle->current_position.state = ENCODER_STATE_SEARCHING;
    handle->current_position.absolute_position = 0;
    handle->current_position.relative_position_change = 0;
    handle->current_position.confidence_level = 0;

    // 重置搜索管理器
    handle->search_manager->reset();

#ifdef LOGD
    LOGD(absolute_encoder_log_level, "Encoder: Switched to search mode\n");
#endif
}