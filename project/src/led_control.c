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
	void Led_control_by_count(unsigned char count)
	{
		/* 先关闭所有LED */
		at32_led_off(LED2);
		at32_led_off(LED3);
		at32_led_off(LED4);
		switch(count)
		{
			case 1:
				at32_led_on(LED2);
			//printf("LED Status: 1 LED on (LED2)\r\n");
				break;
			case 2:
				at32_led_on(LED2);
				at32_led_on(LED3);
		//	printf("LED Status: 2 LEDs on (LED2+LED3)\r\n");
				break;
			case 3:
				at32_led_on(LED2);
				at32_led_on(LED3);
				at32_led_on(LED4);
			//printf("LED Status: 3 LEDs on (LED2+LED3+LED4)\r\n");
				break;
			
			default:
			//	printf("LED Status: All LEDs off\r\n");
				break;
		}
	}
	
	