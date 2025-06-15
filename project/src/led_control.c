/**
  **************************************************************************
  * @file     led_control.c
  * @brief    LED控制函数
  **************************************************************************
  */
	
#include "at32f403_board.h"
#include "at32f403_clock.h"
#include "led_control.h"
#include "multi_button.h"
#include "logger.h"

static Button user_button;



// LED配置结构体
typedef struct {
    gpio_type* gpio_port;
    uint16_t gpio_pin;
    crm_periph_clock_type clock;
} led_config_t;

// 风速LED配置数组（根据您的表格）
static const led_config_t speed_leds[6] = {
    {GPIOB, GPIO_PINS_8,  CRM_GPIOB_PERIPH_CLOCK},   // SPEED_LED1 - PB8
    {GPIOC, GPIO_PINS_13, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED2 - PC13  
    {GPIOC, GPIO_PINS_14, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED3 - PC14
    {GPIOC, GPIO_PINS_15, CRM_GPIOC_PERIPH_CLOCK},   // SPEED_LED4 - PC15
    {GPIOD, GPIO_PINS_0,  CRM_GPIOD_PERIPH_CLOCK},   // SPEED_LED5 - PD0
    {GPIOD, GPIO_PINS_1,  CRM_GPIOD_PERIPH_CLOCK}    // SPEED_LED6 - PD1
};

// PWM占空比配置表 (0-100%)
static const uint16_t pwm_duty_table[7] = {
    0,    // 关闭
    20,   // 1档 - 20%
    35,   // 2档 - 35%
    50,   // 3档 - 50%
    65,   // 4档 - 65%
    80,   // 5档 - 80%
    100   // 6档 - 100%
};

static speed_level_t current_speed = SPEED_OFF;


/**
 * @brief 初始化风速指示LED
 */
void speed_leds_init(void)
{
    gpio_init_type gpio_init_struct;
    
    // 配置GPIO参数
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    
    // 初始化每个风速LED
    for(int i = 0; i < 6; i++) {
        // 使能GPIO时钟
        crm_periph_clock_enable(speed_leds[i].clock, TRUE);
        
        // 配置GPIO
        gpio_init_struct.gpio_pins = speed_leds[i].gpio_pin;
        gpio_init(speed_leds[i].gpio_port, &gpio_init_struct);
        
        // 初始状态关闭
        gpio_bits_reset(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    LOG_ERROR("Speed LEDs Initialized\r\n");
}

/**
 * @brief 更新风速LED显示
 * @param level: 风速等级
 */
void update_speed_leds(speed_level_t level)
{
    // 先关闭所有风速LED
    for(int i = 0; i < 6; i++) {
        gpio_bits_reset(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    // 根据风速等级点亮相应数量的LED
    for(int i = 0; i < level && i < 6; i++) {
        gpio_bits_set(speed_leds[i].gpio_port, speed_leds[i].gpio_pin);
    }
    
    LOG_ERROR("Speed LEDs updated to level %d\r\n", level);
}

/**
 * @brief 设置PWM占空比
 * @param level: 风速等级
 */
void set_fan_pwm(speed_level_t level)
{
    uint16_t duty = pwm_duty_table[level];
    
    // 调用PWM设置函数
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
 * @brief 切换到下一个风速等级
 */
void next_speed_level(void)
{
    current_speed++;
    
    // 循环控制：超过最大档位则回到关闭状态
    if(current_speed > SPEED_MAX) {
        current_speed = SPEED_OFF;
    }
    
    // 更新LED显示和PWM输出
    update_speed_leds(current_speed);
    set_fan_pwm(current_speed);
    
    LOG_ERROR("Speed changed to level %d\r\n", current_speed);
}

/**
 * @brief 获取当前风速等级
 */
speed_level_t get_current_speed(void)
{
    return current_speed;
}

/**
 * @brief 手动设置风速等级
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


//按键处理函数
uint8_t read_button_gpio(uint8_t button_id) {
	uint8_t key_value = RESET;
	if (button_id == USER_BUTTON) {
		key_value = gpio_input_data_bit_read(USER_BUTTON_PORT, USER_BUTTON_PIN);
	}
	return key_value; // 默认返回0
}

//按键按下事件回调函数
void button_press_down_callback(Button* handle)
{
	
	LOG_ERROR("Button Press Down\r\n");
}

//按键释放事件回调函数
void button_press_up_callback(Button* handle)
{
	LOG_ERROR("Button Press Up\r\n");
}

// 单击事件回调函数 - 修改为风速控制
void button_single_click_callback(Button* handle)
{
    // 切换到下一个风速档位
    next_speed_level();
    LOG_ERROR("Single click - Speed level changed\r\n");
}

// 长按事件回调函数 - 新增紧急停止功能
void button_long_press_callback(Button* handle)
{
    // 长按直接关闭风扇
    current_speed = SPEED_OFF;
    update_speed_leds(current_speed);
    set_fan_pwm(current_speed);
    LOG_ERROR("Long press - Fan stopped (Emergency stop)\r\n");
}

//按键和LED初始化
void led_button_init(void)
{

	 // 初始化风速指示LED
    speed_leds_init();
	
	// 初始化用户按键
	button_init(&user_button, read_button_gpio, 1, USER_BUTTON);

	// 绑定按键事件回调函数
//	button_attach(&user_button, BTN_PRESS_DOWN, button_press_down_callback);
//	button_attach(&user_button, BTN_PRESS_UP, button_press_up_callback);
	button_attach(&user_button, BTN_SINGLE_CLICK, button_single_click_callback);
	

	// 启动按钮处理
	button_start(&user_button);
	
	// 初始化风速状态
  current_speed = SPEED_OFF;
  update_speed_leds(current_speed);
  set_fan_pwm(current_speed);
	
	LOG_ERROR("LED and Button Initialized\r\n");
}

/**
 * @brief 获取风速状态信息（用于调试或显示）
 */
void print_speed_status(void)
{
    LOG_ERROR("Current Speed Level: %d, PWM Duty: %d%%\r\n", 
              current_speed, pwm_duty_table[current_speed]);
}

/*
*@brief 设置PWM占控比函数
*/
uint16_t drive_pwm_set(uint16_t duty)
{
    uint16_t ccr2_val = duty * 10;  // 0-100 映射到 0-1000
    if(ccr2_val > 999) ccr2_val = 999;  // 限制在Period范围内
    return ccr2_val;
}