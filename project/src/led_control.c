/**
  **************************************************************************
  * @file     led_control.c
  * @brief    LED���ƺ���
  **************************************************************************
  */
	
#include "at32f403_board.h"
#include "at32f403_clock.h"
#include "led_control.h"
/**
  * @brief  ���ݼ���ֵ����LED״̬
  * @param  count: LED�������� (0=ȫ��, 1=��1��, 2=��2��, 3=��3��)
  * @retval none
  */
	void Led_control_by_count(unsigned char count)
	{
		/* �ȹر�����LED */
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
	
	