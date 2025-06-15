/**
  **************************************************************************
  * @file     led_control.h
  * @brief    LED控制头文件
  **************************************************************************
  */


#ifndef __LED_CONTROL_H
#define __LED_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include<stdint.h> 

void Led_control_by_count(unsigned char count);


// 风速等级定义
typedef enum {
    SPEED_OFF = 0,
    SPEED_1,
    SPEED_2, 
    SPEED_3,
    SPEED_4,
    SPEED_5,
    SPEED_6,
    SPEED_MAX = SPEED_6
} speed_level_t;

// 函数声明

/**
 * @brief 按键和LED系统初始化
 */
void led_button_init(void);

/**
 * @brief 按键处理任务 - 需要在主循环中定期调用
 */
void button_process_task(void);

/**
 * @brief 风速LED初始化
 */
void speed_leds_init(void);

/**
 * @brief 更新风速LED显示
 * @param level: 风速等级 (0-6)
 */
void update_speed_leds(speed_level_t level);

/**
 * @brief 设置风扇PWM
 * @param level: 风速等级 (0-6)
 */
void set_fan_pwm(speed_level_t level);

/**
 * @brief 切换到下一个风速等级
 */
void next_speed_level(void);

/**
 * @brief 获取当前风速等级
 * @retval 当前风速等级
 */
speed_level_t get_current_speed(void);

/**
 * @brief 手动设置风速等级
 * @param level: 目标风速等级 (0-6)
 */
void set_speed_level(speed_level_t level);

/**
 * @brief 打印当前风速状态信息
 */
void print_speed_status(void);

uint16_t drive_pwm_set(uint16_t duty);


#ifdef __cplusplus
}

#endif

#endif /* __LED_CONTROL_H */