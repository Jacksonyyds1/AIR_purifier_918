/**
  **************************************************************************
  * @file     led_control.h
  * @brief    LED����ͷ�ļ�
  **************************************************************************
  */


#ifndef __LED_CONTROL_H
#define __LED_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include<stdint.h> 

void Led_control_by_count(unsigned char count);


// ���ٵȼ�����
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

// ��������

/**
 * @brief ������LEDϵͳ��ʼ��
 */
void led_button_init(void);

/**
 * @brief ������������ - ��Ҫ����ѭ���ж��ڵ���
 */
void button_process_task(void);

/**
 * @brief ����LED��ʼ��
 */
void speed_leds_init(void);

/**
 * @brief ���·���LED��ʾ
 * @param level: ���ٵȼ� (0-6)
 */
void update_speed_leds(speed_level_t level);

/**
 * @brief ���÷���PWM
 * @param level: ���ٵȼ� (0-6)
 */
void set_fan_pwm(speed_level_t level);

/**
 * @brief �л�����һ�����ٵȼ�
 */
void next_speed_level(void);

/**
 * @brief ��ȡ��ǰ���ٵȼ�
 * @retval ��ǰ���ٵȼ�
 */
speed_level_t get_current_speed(void);

/**
 * @brief �ֶ����÷��ٵȼ�
 * @param level: Ŀ����ٵȼ� (0-6)
 */
void set_speed_level(speed_level_t level);

/**
 * @brief ��ӡ��ǰ����״̬��Ϣ
 */
void print_speed_status(void);

uint16_t drive_pwm_set(uint16_t duty);


#ifdef __cplusplus
}

#endif

#endif /* __LED_CONTROL_H */