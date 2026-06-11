/**
 * @file    sys_timer.h
 * @author  Mem 2
 * @brief   System tick timer module. Provides 1ms resolution for all modules.
 * @note    Call SysTimer_Init() once in main() before any other module init.
 */
#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include <stdint.h>

/* Auto-select HAL or bare-metal header */
#ifdef USE_HAL_DRIVER
    #include "stm32f4xx_hal.h"
#else
    #include "stm32f4xx.h"
#endif

extern volatile uint32_t g_tick_ms;

/**
 * @brief  Configures SysTick for 1ms interrupt (bare-metal only).
 *         No-op when USE_HAL_DRIVER is defined — HAL_Init() handles it.
 * @param  cpu_freq_hz  CPU frequency in Hz. Pass SystemCoreClock.
 */
void SysTimer_Init(uint32_t cpu_freq_hz);

/**
 * @brief  Increments tick counter by 1. Call inside SysTick_Handler.
 */
void SysTimer_Increment(void);

/**
 * @brief  Returns current system tick in milliseconds.
 * @retval Tick count since startup (uint32_t, wraps after ~49 days).
 */
uint32_t SysTimer_GetTick(void);

#endif /* SYS_TIMER_H */
