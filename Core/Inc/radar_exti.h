/**
 * @file    radar_exti.h
 * @author  Mem 2
 * @brief   Driver for RCWL-0516 Radar sensor using EXTI and bare-metal registers.
 * @date    2026
 */

#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Macro Definitions                                                 */
/* ------------------------------------------------------------------ */
#define RADAR_PRESENCE   1U   /**< Motion detected and validated */
#define RADAR_ABSENCE    0U   /**< No motion detected */

/* ------------------------------------------------------------------ */
/* External Global Variables                                         */
/* ------------------------------------------------------------------ */
/**
 * @brief Global system tick counter (incremented every 1ms in SysTick_Handler).
 * Shared across all modules for non-blocking timing.
 */
extern volatile uint32_t g_tick_ms;

/* ------------------------------------------------------------------ */
/* Function Prototypes                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initializes GPIOA Pin 0 as Input Pull-Down and configures EXTI0.
 * Sets up NVIC with priority 2 for external interrupt handling.
 * @note   This function uses direct register access (bare-metal).
 */
void Radar_EXTI_Init(void);

/**
 * @brief  ISR Callback function executed when an EXTI0 trigger occurs.
 * Sets the raw trigger flag to acknowledge motion detection.
 */
void Radar_EXTI_Callback(void);

/**
 * @brief  Processes the raw radar trigger using a time filter and a hold timer.
 * @retval RADAR_PRESENCE if motion is validated (sustained >= 60ms),
 * RADAR_ABSENCE if no motion or noise detected.
 * @note   Must be polled frequently (every 10-50ms) in the main loop.
 */
uint8_t Radar_Is_Detected(void);

/**
 * @brief  Resets all internal filter flags and timer states.
 * Should be called when transitioning back to IDLE state.
 */
void Radar_ClearPresence(void);

#endif /* RADAR_EXTI_H */
