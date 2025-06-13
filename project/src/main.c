/**
  **************************************************************************
  * @file     main.c
  * @brief    main program
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

#include "at32f403_board.h"
#include "at32f403_clock.h"
#include "logger.h"
#include "ihastek.h"
#include "TimeEvent.h"
#include "multi_button.h"
/** @addtogroup AT32F403_periph_template
  * @{
  */

/** @addtogroup 403_LED_toggle LED_toggle
  * @{
  */

#define DELAY                            100
#define FAST                             1
#define SLOW                             4

uint8_t g_speed = FAST;

static uint8_t led_count = 0;

void button_exint_init(void);
void button_isr(void);



/**
 * @brief  定时器3中断服务函数
 */
void TMR3_GLOBAL_IRQHandler(void)
{
    if(tmr_flag_get(TMR3, TMR_OVF_FLAG) != RESET)
    {
        /* 清除中断标志 */
        tmr_flag_clear(TMR3, TMR_OVF_FLAG);
        
        /* 调用软件定时器1ms处理函数 */
        fw_timer_event_isr_1ms();
    }
}

void Printf_task(void)
{

	
	LOG_ERROR("hello world!\r\n");
	
	
	fw_timer_event_ActiveTimerEvent(1000,Printf_task);
}

void Led_task(void)
{
  button_ticks();
  
  LOG_ERROR("LED task running\r\n");
}

/**
  * @brief  main function.
  * @param  none
  * @retval none
  */
int main(void)
{
	
	log_init();
	
  system_clock_config();

  at32_board_init();

  uart_print_init(115200);
	
	  /* 初始化1ms定时器中断 */
    timer_1ms_init();
	
	fw_timer_event_CancelAllTimerEvent();
	

	printf("hello world!\r\n");
	
	//add 500ms task
	fw_timer_event_ActiveTimerEvent(1000,Printf_task);
  fw_timer_event_ActiveTimerEvent(10,Led_task);
	
	
  while(1)
  {
		fw_timer_event_Handler();
  }
}

/**
  * @}
  */

/**
  * @}
  */
