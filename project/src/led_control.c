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

//LED2״̬
static uint8_t led2_status = 0;   //0 - �ر�, 1 - ��

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

//�����¼��ص����� - LED ����״̬
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

//������LED��ʼ��
void led_button_init(void)
{
	// ��ʼ��LED2
	at32_led_init(LED2);
	at32_led_off(LED2); // Ĭ�Ϲر�LED2

	// ��ʼ���û�����
	button_init(&user_button, read_button_gpio, 1, USER_BUTTON);

	// �󶨰����¼��ص�����
//	button_attach(&user_button, BTN_PRESS_DOWN, button_press_down_callback);
//	button_attach(&user_button, BTN_PRESS_UP, button_press_up_callback);
	button_attach(&user_button, BTN_SINGLE_CLICK, button_single_click_callback);

	// ������ť����
	button_start(&user_button);
	LOG_ERROR("LED and Button Initialized\r\n");
}



