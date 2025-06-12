# 绝对编码器组件

## 概述
本组件实现绝对编码器功能，通过光电开关检测环形码盘上的编码图案，将光信号转换为数字信号(0或1)，实现位置的唯一确定和实时跟踪。系统采用二叉树搜索算法快速定位，并通过智能跟踪模式实现高精度位置监控。

## 核心特性
- **智能搜索算法**：使用二叉树快速定位码盘位置
- **双模式运行**：支持搜索模式和跟踪模式自动切换
- **高精度跟踪**：实时位置跟踪和状态监控
- **信号预处理**：内置迟滞滤波和容错机制
- **电机集成**：支持电机步进反馈和误差校正
- **C语言接口**：提供标准C接口，便于嵌入式系统集成

## 组件架构

### 核心文件
- `absolute_encoder.h` - C语言接口定义，包括数据结构和API声明
- `absolute_encoder.cpp` - 核心逻辑实现，状态机管理和模式切换

### 核心算法模块
- `encoder_map.hpp/cpp` - 编码器图案管理和二叉树搜索
- `smart_search_manager.hpp/cpp` - 智能搜索算法，位置定位逻辑
- `position_tracker.hpp/cpp` - 位置跟踪模式，实时位置更新
- `input_preprocessor.hpp/cpp` - 信号预处理，滤波和电机反馈处理

### 测试和工具
- `test/` - 完整的单元测试和集成测试套件
  - `test_smart_search.cpp` - 智能搜索管理器测试，包含大规模随机测试
  - `test_common.cpp/hpp` - 统一测试工具和验证接口
  - 支持搜索偏移、重复序列验证和失败用例分析

## 使用说明
待完善...

## 系统工作原理

### 1. 编码器映射 (EncoderMap)
- 存储完整的码盘图案数据
- 构建二叉树搜索结构，支持快速位置查找
- 提供模式匹配和位置验证功能

### 2. 智能搜索管理 (SmartSearchManager)  
- 维护输入信号历史缓冲区
- 实现滑动窗口搜索算法，支持搜索偏移重试
- 支持前进/后退双向搜索
- 智能筛选候选位置，提高搜索效率
- 返回实际匹配序列长度和偏移信息

### 3. 位置跟踪 (PositionTracker)
- 基于当前位置和运动方向进行预测验证
- 简化的位值匹配算法，避免复杂序列验证
- 连续失败检测和自动恢复机制
- 支持方向变化的快速适应

### 4. 信号预处理 (InputPreprocessor)
- 迟滞滤波消除信号抖动
- 电机步进信号处理和单位换算
- 累积误差监控和补偿
- 多种滤波策略支持

## API接口

### 编码器图案管理
```cpp
// 创建编码器图案映射，支持内存池模式
EncoderMap(const uint8_t* pattern, uint16_t pattern_length, uint8_t max_search_depth);
EncoderMap(const uint8_t* pattern, uint16_t pattern_length, uint8_t max_search_depth,
           TreeNode* node_pool, uint16_t pool_size);

// 构建搜索树和序列索引
void build_search_tree();

// 搜索接口
SearchResult absolute_search(TreeNode*& current_node, uint8_t bit, 
                           SearchDirection direction, uint16_t& position);
TreeNode* start_search(uint8_t bit, SearchDirection direction);
```

### 智能搜索管理接口
```cpp
// 基础搜索接口
SearchResult add_bit_and_search(uint8_t bit, SearchDirection direction, uint16_t& position);

// 带详细信息的搜索接口 - 返回实际匹配长度和偏移
SearchResult add_bit_and_search(uint8_t bit, SearchDirection direction, uint16_t& position, 
                               uint8_t& sequence_length, uint8_t& start_offset);

// 重置搜索状态
void reset();

// 获取搜索统计信息
struct SearchStats {
    uint32_t total_searches;        // 总搜索次数
    uint32_t successful_searches;   // 成功搜索次数
    uint32_t retry_count;          // 重试次数
    uint32_t max_sequence_length;  // 最大成功序列长度
};
const SearchStats& get_stats() const;
```

### 编码器对象管理
```c
// 编码器配置结构
typedef struct {
    uint16_t motor_steps_per_unit;      // 电机步数与编码器单位的比率
    uint8_t search_timeout_seconds;     // 搜索超时时间(秒)，0表示永不超时
    uint8_t tracking_lost_threshold;    // 跟踪失败阈值
    uint64_t (*sys_tick_get)(void);    // 系统时钟获取函数指针
} encoder_config_t;

// 创建编码器对象
encoder_handle_t create_encoder(encoder_map_handle_t map_handle, 
                               const encoder_config_t* config);

// 销毁编码器对象
void destroy_encoder(encoder_handle_t handle);
```

### 信号处理
```c
// 处理编码器信号
encoder_result_t process_encoder_signal(encoder_handle_t handle, 
                                       uint8_t signal_bit, 
                                       encoder_position_t* position);

// 处理电机步进信号
encoder_result_t process_motor_steps(encoder_handle_t handle, 
                                   uint16_t steps, 
                                   int8_t direction);
```

### 状态查询和控制
```c
// 获取当前位置信息
bool get_current_position(encoder_handle_t handle, 
                         encoder_position_t* position);

// 强制切换到搜索模式
bool force_search_mode(encoder_handle_t handle);

// 重置编码器状态
bool reset_encoder(encoder_handle_t handle);

// 获取统计信息
bool get_encoder_stats(encoder_handle_t handle, 
                      encoder_stats_t* stats);

### 日志接口
```c
// 设置日志级别
void absolute_encoder_set_log_level(int level);

// 日志级别定义
typedef enum {
    LOG_LEVEL_ERROR = 0,    // 错误信息
    LOG_LEVEL_WARN,         // 警告信息
    LOG_LEVEL_INFO,         // 一般信息
    LOG_LEVEL_DEBUG,        // 调试信息
    LOG_LEVEL_VERBOSE       // 详细信息
} LogLevel;

// 分级日志宏（在C++模块内部使用）
LOGE(level, fmt, ...);      // 错误日志
LOGW(level, fmt, ...);      // 警告日志
LOGI(level, fmt, ...);      // 信息日志
LOGD(level, fmt, ...);      // 调试日志
LOGV(level, fmt, ...);      // 详细日志

// 条件日志宏
LOGE_IF(cond, level, fmt, ...);  // 条件错误日志
LOGW_IF(cond, level, fmt, ...);  // 条件警告日志
LOGI_IF(cond, level, fmt, ...);  // 条件信息日志
LOGD_IF(cond, level, fmt, ...);  // 条件调试日志
LOGV_IF(cond, level, fmt, ...);  // 条件详细日志
```

**日志特性：**
- 自动添加时间戳、文件名、行号、函数名
- 支持分级输出控制
- 线程安全的日志输出
- 条件日志宏减少性能开销
- 标准输出格式便于日志分析

**使用示例：**
```c
// 设置日志级别为调试模式
absolute_encoder_set_log_level(LOG_LEVEL_DEBUG);

// 创建编码器后，内部会输出相关日志
encoder_handle_t encoder = create_encoder(map, &config);

// 在处理过程中会自动输出状态日志
encoder_result_t result = process_encoder_signal(encoder, signal, &position);
```

**日志输出格式：**
```
[2024-12-20 14:30:25] [I] encoder_map.cpp:123 build_search_tree() Search tree built with 256 nodes
[2024-12-20 14:30:25] [D] smart_search_manager.cpp:89 add_bit_and_search() Processing bit: 1, buffer size: 5
[2024-12-20 14:30:25] [I] absolute_encoder.cpp:234 transition_to_tracking_mode() Switched to tracking mode at position 142
```

## 数据结构

### 位置信息
```c
typedef struct {
    encoder_state_t state;              // 编码器状态
    uint32_t absolute_position;         // 绝对位置
    int32_t relative_position_change;   // 相对位置变化
    uint8_t confidence_level;           // 置信度(0-100)
} encoder_position_t;
```

### 统计信息
```c
typedef struct {
    uint32_t total_signals_processed;   // 总处理信号数
    uint32_t search_attempts;           // 搜索尝试次数
    uint32_t tracking_updates;          // 跟踪更新次数
    uint32_t error_count;               // 错误计数
    uint32_t runtime_ticks;             // 运行时间(毫秒)
} encoder_stats_t;
```

### 状态枚举
```c
typedef enum {
    ENCODER_STATE_UNINITIALIZED = 0,   // 未初始化
    ENCODER_STATE_SEARCHING,           // 搜索模式
    ENCODER_STATE_TRACKING,            // 跟踪模式
    ENCODER_STATE_ERROR                // 错误状态
} encoder_state_t;

typedef enum {
    ENCODER_RESULT_OK = 0,             // 操作成功
    ENCODER_RESULT_SEARCHING,          // 搜索中
    ENCODER_RESULT_POSITION_FOUND,     // 找到位置
    ENCODER_RESULT_TRACKING_UPDATED,   // 跟踪位置更新
    ENCODER_RESULT_ERROR_TIMEOUT,      // 搜索超时
    ENCODER_RESULT_ERROR_LOST_TRACKING,// 跟踪丢失
    ENCODER_RESULT_ERROR_INVALID_SIGNAL// 无效信号
} encoder_result_t;
```

## 使用示例

### 基本使用流程
```c
#include "absolute_encoder/absolute_encoder.h"

// 1. 准备码盘图案数据
const uint8_t pattern[] = {0,1,0,0,1,1,0,1, /*...*/ };
const uint32_t pattern_length = sizeof(pattern);

// 2. 创建编码器图案映射
encoder_map_handle_t map = create_encoder_map(pattern, pattern_length, 8);

// 3. 配置编码器参数
encoder_config_t config = {
    .motor_steps_per_unit = 100,
    .search_timeout_seconds = 10,
    .tracking_lost_threshold = 3,
    .sys_tick_get = sys_tick_get,
};

// 4. 创建编码器对象
encoder_handle_t encoder = create_encoder(map, &config);

// 5. 处理信号循环
encoder_position_t position;
while (running) {
    uint8_t signal = read_sensor(); // 读取光电开关信号
    
    encoder_result_t result = process_encoder_signal(encoder, signal, &position);
    
    switch (result) {
        case ENCODER_RESULT_SEARCHING:
            printf("搜索中，置信度: %d%%\n", position.confidence_level);
            break;
        case ENCODER_RESULT_POSITION_FOUND:
            printf("找到位置: %u\n", position.absolute_position);
            break;
        case ENCODER_RESULT_TRACKING_UPDATED:
            printf("位置更新: %u (变化: %d)\n", 
                   position.absolute_position, position.relative_position_change);
            break;
        case ENCODER_RESULT_ERROR_TIMEOUT:
            printf("搜索超时，强制重新搜索\n");
            force_search_mode(encoder);
            break;
    }
}

// 6. 清理资源
destroy_encoder(encoder);
destroy_encoder_map(map);
```

### 电机集成示例
```c
// 处理电机步进反馈
void handle_motor_steps(encoder_handle_t encoder, int steps) {
    int8_t direction = (steps > 0) ? 1 : -1;
    uint16_t abs_steps = abs(steps);
    
    encoder_result_t result = process_motor_steps(encoder, abs_steps, direction);
    if (result != ENCODER_RESULT_OK) {
        printf("电机步进处理失败\n");
    }
}
```

## 配置和调优

### 编码器图案设计建议
- **图案长度**：建议64-512位，平衡搜索速度和分辨率
- **搜索深度**：8-12位较为合适，过深影响性能，过浅影响搜索可靠性
- **图案特性**：确保任意连续子序列在图案中唯一出现

### 参数调优指南
```c
encoder_config_t config = {
    // 电机步数比率：根据实际机械结构调整
    .motor_steps_per_unit = 100,        // 100步/编码器单位
    
    // 搜索超时：平衡搜索时间和系统响应
    .search_timeout_seconds = 5,        // 5秒超时
    
    // 跟踪失败阈值：容错能力配置
    .tracking_lost_threshold = 3,       // 连续3次失败切换搜索模式

    // 系统时钟接口：超时功能依赖
    .sys_tick_get = sys_tick_get,
};
```

### 性能优化建议
1. **合理设置搜索深度**：深度过大会影响搜索树构建和内存使用
2. **优化信号采集频率**：避免过高频率导致的CPU负载
3. **调整容错阈值**：根据实际噪声水平设置合适的失败阈值
4. **使用电机反馈**：结合电机步进信号提高跟踪精度

## 性能特性和优化

### 搜索性能
- **二叉树深度优化**：根据序列唯一性动态调整搜索深度
- **滑动窗口重试**：搜索失败时自动尝试不同的起始偏移
- **智能缓存**：维护搜索状态，避免重复计算
- **内存池支持**：可选的预分配内存池，减少动态内存分配

### 验证准确性
- **位置偏移校正**：正确处理滑动窗口导致的位置偏移
- **重复序列容错**：识别并允许德布鲁因序列中的合理重复
- **深度信息利用**：使用搜索树叶子节点的实际深度进行验证
- **统一验证逻辑**：所有测试使用同一验证接口，确保结果一致性

### 测试覆盖率
- **序列长度覆盖**：1-12位序列长度的全面测试
- **方向覆盖**：正向和反向搜索的完整验证
- **边界条件**：环形码盘边界处理的专项测试
- **大规模验证**：10,000+次随机测试确保算法稳定性

## 故障排除

### 常见问题
1. **搜索超时**
   - 检查码盘图案是否正确
   - 确认光电开关信号质量
   - 调整搜索超时时间

2. **跟踪频繁丢失**
   - 检查信号噪声水平
   - 调整跟踪失败阈值
   - 验证运动方向检测

3. **位置精度不够**
   - 确认码盘安装精度
   - 检查机械间隙和振动
   - 优化信号滤波参数

### 调试技巧
```c
// 设置日志级别为调试模式
absolute_encoder_set_log_level(LOG_LEVEL_DEBUG);

// 定期检查统计信息
encoder_stats_t stats;
get_encoder_stats(encoder, &stats);
printf("处理信号: %u, 搜索: %u, 错误: %u\n", 
       stats.total_signals_processed, stats.search_attempts, stats.error_count);

// 运行时动态调整日志级别
absolute_encoder_set_log_level(LOG_LEVEL_INFO);  // 减少日志输出

// 在生产环境中关闭日志
absolute_encoder_set_log_level(-1);  // 完全关闭日志
```

**日志级别建议：**
- **开发阶段**：使用`LOG_LEVEL_DEBUG`获取详细调试信息
- **测试阶段**：使用`LOG_LEVEL_INFO`关注关键状态变化
- **生产环境**：使用`LOG_LEVEL_ERROR`或关闭日志以提高性能

## 开发工具

### 码盘图案生成工具
位于 `pytools/debruijn_generator.py` 的德布鲁因序列生成器：

```bash
cd pytools
python3 debruijn_generator.py
```

**功能特性：**
- 支持k值1-10的德布鲁因序列生成
- 交互式命令行界面
- 自动验证生成序列的正确性
- 支持保存为C++头文件格式
- 序列唯一性验证

**使用示例：**
```
$ python3 debruijn_generator.py
德布鲁因序列生成器
支持k值范围：1-10
请输入k值 (1-10): 8
生成参数: k = 8
德布鲁因序列长度: 2^8 = 256 位
正在生成德布鲁因序列...
✓ 生成成功! 序列长度: 256
✓ 验证通过: 找到256个唯一的8-mer子序列
是否保存到文件? (Y/n): Y
请输入文件名 (默认: debruijn_k8.txt): pattern.h
✓ 序列已保存到 pattern.h
```

**输出格式：**
生成的文件包含C++兼容的数组定义：
```cpp
// 德布鲁因序列 B(8,2) - 长度: 256
const uint8_t pattern_data[] = {
    1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,
    1,1,1,1,0,1,1,0,1,1,1,0,1,0,1,1,
    // ...
};
const uint16_t pattern_length = 256;
```

**德布鲁因序列特性：**
- 每个长度为k的二进制子序列在整个序列中恰好出现一次
- 序列长度为2^k位
- 适用于绝对编码器的位置唯一确定
- k=8时提供256个位置的分辨率

**选择k值建议：**
- k=6：64位序列，适合小型码盘
- k=7：128位序列，平衡分辨率和搜索速度
- k=8：256位序列，高分辨率应用
- k=9+：512位以上，大型高精度系统

## 编译和使用

### 编译组件
```bash
# 进入absolute_encoder目录
cd absolute_encoder

# 编译发布版本（生成 absolute_encoder.o）
make release

# 编译调试版本（生成 absolute_encoder_debug.o）
make debug

# 编译静态库版本（生成 libabsolute_encoder.a）
make lib

# 清理编译文件
make clean

# 查看帮助信息
make help
```

### 集成到项目
```bash
# 方法1：直接链接目标文件
gcc your_main.c absolute_encoder.o -o your_program -lstdc++

# 方法2：使用静态库
gcc your_main.c -L. -labsolute_encoder -o your_program -lstdc++

# 方法3：在你的Makefile中包含
your_program: your_main.o absolute_encoder.o
	gcc $^ -o $@ -lstdc++
```

**编译要求：**
- C++11兼容编译器（g++ 4.8+）
- 支持C99的C编译器
- 需要链接C++标准库（-lstdc++）

**编译选项说明：**
- 发布版本：优化编译（-O2），适合生产环境
- 调试版本：包含调试符号（-g），支持gdb调试
- 静态库版本：便于多项目复用和分发

## 许可证和版权
本组件遵循 MIT 许可证，允许自由使用、修改和分发。

## 技术支持
如有技术问题或建议，请提交Issue或联系开发团队。
