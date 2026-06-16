/**
 * @file    sys_timer.c
 * @author  Mem 2
 * @brief   System tick timer implementation. Compatible with HAL and bare-metal.
 * @note    Automatically selects init strategy based on USE_HAL_DRIVER macro.
 */
#include "sys_timer.h"

volatile uint32_t g_tick_ms = 0;

void SysTimer_Init(uint32_t cpu_freq_hz)
{
#ifdef USE_HAL_DRIVER
    /*
     * HAL project: SysTick already configured inside HAL_Init() in main.c.
     * Skip re-configuration to avoid overwriting HAL's timer settings.
     */
    (void)cpu_freq_hz;
#else
    /*
     * Bare-metal project: No HAL present, configure SysTick manually.
     * Generates a 1ms interrupt using the provided CPU frequency.
     */
    SysTick_Config(cpu_freq_hz / 1000U); //16MHz
#endif
}

void SysTimer_Increment(void)
{
    g_tick_ms++;
}

uint32_t SysTimer_GetTick(void)
{
    return g_tick_ms;
}
