/**
  **************************************************************************
  * @file     led_control.c
  * @brief    LED控制函数
  **************************************************************************
  */
	
#include "at32f403_board.h"
#include "at32f403_clock.h"
#include "led_control.h"
/**
  * @brief  根据计数值控制LED状态
  * @param  count: LED点亮数量 (0=全灭, 1=亮1个, 2=亮2个, 3=亮3个)
  * @retval none
  */
	void Led_control_by_count(uint8_t count)
	{
		/* 先关闭所有LED */
		at32_led_off(LED2);
		at32_led_off(LED3);
		at32_led_off(LED4);
		switch(count)
		{
			case 1:
				at32_led_on(LED2);
				printf("LED状态：1个LED灯点亮（LED2）\r\n");
				break;
			case 2:
				at32_led_on(LED2);
				at32_led_on(LED3);
				printf("LED状态: 2个LED点亮 (LED2+LED3)\r\n");
				break;
			case 3:
				at32_led_on(LED2);
				at32_led_on(LED3);
				at32_led_on(LED4);
				printf("LED状态: 2个LED点亮 (LED2+LED3+LED4)\r\n");
				break;
			
			default:
				printf("LED状态：所有LED关闭\r\n");
				break;
		}
	}
	
	