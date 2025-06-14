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

//LED2状态
static uint8_t led2_status = 0;   //0 - 关闭, 1 - 打开

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

//单击事件回调函数 - LED 开关状态
void button_single_click_callback(Button* handle)
{
	led2_status = !led2_status;
  
    if (led2_status) {
        at32_led_on(LED2);
        LOG_ERROR("LED2 turned ON (Single Click)\r\n");
    } else {
        at32_led_off(LED2);
        LOG_ERROR("LED2 turned OFF (Single Click)\r\n");
    }
	LOG_ERROR("button_single_click_callback\r\n");
}

//按键和LED初始化
void led_button_init(void)
{
	// 初始化LED2
	at32_led_init(LED2);
	at32_led_off(LED2); // 默认关闭LED2

	// 初始化用户按键
	button_init(&user_button, read_button_gpio, 1, USER_BUTTON);

	// 绑定按键事件回调函数
//	button_attach(&user_button, BTN_PRESS_DOWN, button_press_down_callback);
//	button_attach(&user_button, BTN_PRESS_UP, button_press_up_callback);
	button_attach(&user_button, BTN_SINGLE_CLICK, button_single_click_callback);

	// 启动按钮处理
	button_start(&user_button);
	LOG_ERROR("LED and Button Initialized\r\n");
}



