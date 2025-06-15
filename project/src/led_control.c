/**
  **************************************************************************
  * @file     led_control.c
  * @brief    LED���ƺ���
  **************************************************************************
  */
	
#include "at32f403_board.h"
#include "at32f403_clock.h"
#include "led_control.h"
#include "multi_button.h"
#include "logger.h"

static Button user_button;



// LED���ýṹ��
typedef struct {
    gpio_type* gpio_port;
    uint16_t gpio_pin;
    crm_periph_clock_type clock;
} led_config_t;

// ����LED�������飨�������ı��
static const led_config_t speed_leds[6] = {
    {GPIOB, GPIO_PINS_8,  CRM_GPIOB_PERIPH_CLOCK},   // SPEED_LED1 - PB8
    {GPIOC, GPIO_PINS_13, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED2 - PC13  
    {GPIOC, GPIO_PINS_14, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED3 - PC14
    {GPIOC, GPIO_PINS_15, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED4 - PC15
    {GPIOD, GPIO_PINS_0,  CRM_GPIOD_PERIPH_CLOCK},   // SPEED_LED5 - PD0
    {GPIOD, GPIO_PINS_1,  CRM_GPIOD_PERIPH_CLOCK}    // SPEED_LED6 - PD1
};

// PWMռ�ձ����ñ� (0-100%)
static const uint16_t pwm_duty_table[7] = {
    0,    // �ر�
    20,   // 1�� - 20%
    35,   // 2�� - 35%
    50,   // 3�� - 50%
    65,   // 4�� - 65%
    80,   // 5�� - 80%
    100   // 6�� - 100%
};

static speed_level_t current_speed = SPEED_OFF;


/**
 * @brief ��ʼ������ָʾLED
 */
void speed_leds_init(void)
{
    gpio_init_type gpio_init_struct;
    
    // ����GPIO����
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    
    // ��ʼ��ÿ������LED
    for(int i = 0; i < 6; i++) {
        // ʹ��GPIOʱ��
        crm_periph_clock_enable(speed_leds[i].clock, TRUE);
        
        // ����GPIO
        gpio_init_struct.gpio_pins = speed_leds[i].gpio_pin;
        gpio_init(speed_leds[i].gpio_port, &gpio_init_struct);
        
        // ��ʼ״̬�ر�
        gpio_bits_reset(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    LOG_ERROR("Speed LEDs Initialized\r\n");
}

/**
 * @brief ���·���LED��ʾ
 * @param level: ���ٵȼ�
 */
void update_speed_leds(speed_level_t level)
{
    // �ȹر����з���LED
    for(int i = 0; i < 6; i++) {
        gpio_bits_reset(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    // ���ݷ��ٵȼ�������Ӧ������LED
    for(int i = 0; i < level && i < 6; i++) {
        gpio_bits_set(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    LOG_ERROR("Speed LEDs updated to level %d\r\n", level);
}

/**
 * @brief ����PWMռ�ձ�
 * @param level: ���ٵȼ�
 */
void set_fan_pwm(speed_level_t level)
{
    uint16_t duty = pwm_duty_table[level];
    
    // ����PWM���ú���
    tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_2, duty*10);
    
    if(level == SPEED_OFF) {
        PA15_PWM_Enable(FALSE);
        LOG_ERROR("Fan PWM disabled\r\n");
    } else {
        PA15_PWM_Enable(TRUE);
        LOG_ERROR("Fan PWM set to %d%% (Level %d)\r\n", duty, level);
    }
}

/**
 * @brief �л�����һ�����ٵȼ�
 */
void next_speed_level(void)
{
    current_speed++;
    
    // ѭ�����ƣ��������λ��ص��ر�״̬
    if(current_speed > SPEED_MAX) {
        current_speed = SPEED_OFF;
    }
    
    // ����LED��ʾ��PWM���
    update_speed_leds(current_speed);
    set_fan_pwm(current_speed);
    
    LOG_ERROR("Speed changed to level %d\r\n", current_speed);
}

/**
 * @brief ��ȡ��ǰ���ٵȼ�
 */
speed_level_t get_current_speed(void)
{
    return current_speed;
}

/**
 * @brief �ֶ����÷��ٵȼ�
 */
void set_speed_level(speed_level_t level)
{
    if(level <= SPEED_MAX) {
        current_speed = level;
        update_speed_leds(current_speed);
        set_fan_pwm(current_speed);
        LOG_ERROR("Speed manually set to level %d\r\n", level);
    }
}


//����������
uint8_t read_button_gpio(uint8_t button_id) {
	uint8_t key_value = RESET;
	if (button_id == USER_BUTTON) {
		key_value = gpio_input_data_bit_read(USER_BUTTON_PORT, USER_BUTTON_PIN);
	}
	return key_value; // Ĭ�Ϸ���0
}

//���������¼��ص�����
void button_press_down_callback(Button* handle)
{
	
	LOG_ERROR("Button Press Down\r\n");
}

//�����ͷ��¼��ص�����
void button_press_up_callback(Button* handle)
{
	LOG_ERROR("Button Press Up\r\n");
}

// �����¼��ص����� - �޸�Ϊ���ٿ���
void button_single_click_callback(Button* handle)
{
    // �л�����һ�����ٵ�λ
    next_speed_level();
    LOG_ERROR("Single click - Speed level changed\r\n");
}

// �����¼��ص����� - ��������ֹͣ����
void button_long_press_callback(Button* handle)
{
    // ����ֱ�ӹرշ���
    current_speed = SPEED_OFF;
    update_speed_leds(current_speed);
    set_fan_pwm(current_speed);
    LOG_ERROR("Long press - Fan stopped (Emergency stop)\r\n");
}

//������LED��ʼ��
void led_button_init(void)
{

	 // ��ʼ������ָʾLED
    speed_leds_init();
	
	// ��ʼ���û�����
	button_init(&user_button, read_button_gpio, 1, USER_BUTTON);

	// �󶨰����¼��ص�����
//	button_attach(&user_button, BTN_PRESS_DOWN, button_press_down_callback);
//	button_attach(&user_button, BTN_PRESS_UP, button_press_up_callback);
	button_attach(&user_button, BTN_SINGLE_CLICK, button_single_click_callback);
	

	// ������ť����
	button_start(&user_button);
	
	// ��ʼ������״̬
  current_speed = SPEED_OFF;
  update_speed_leds(current_speed);
  set_fan_pwm(current_speed);
	
	LOG_ERROR("LED and Button Initialized\r\n");
}

/**
 * @brief ��ȡ����״̬��Ϣ�����ڵ��Ի���ʾ��
 */
void print_speed_status(void)
{
    LOG_ERROR("Current Speed Level: %d, PWM Duty: %d%%\r\n", 
              current_speed, pwm_duty_table[current_speed]);
}

/*
*@brief ����PWMռ�رȺ���
*/
uint16_t drive_pwm_set(uint16_t duty)
{
    uint16_t ccr2_val = duty * 10;  // 0-100 ӳ�䵽 0-1000
    if(ccr2_val > 999) ccr2_val = 999;  // ������Period��Χ��
    return ccr2_val;
}