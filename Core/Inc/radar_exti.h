/**
 * @file    radar_exti.h
 * @author  Mem 2
 * @brief   Driver for RCWL-0516 Radar sensor using EXTI1 (PA1), bare-metal registers.
 * @date    2026
 */
#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"
#include <stdint.h>
#include "sys_timer.h"

/* ------------------------------------------------------------------ */
/* Hardware Pin Definitions - Edit ONLY here to remap to another pin  */
/* */
/* !!! WARNING: IF YOU CHANGE RADAR_PIN, YOU MUST ALSO:               */
/* 1. Rename ISR in stm32f4xx_it.c to match STM32 Vector Table:     */
/* - Pins 0 to 4:   EXTI0_IRQHandler -> EXTI4_IRQHandler         */
/* - Pins 5 to 9:   EXTI9_5_IRQHandler                           */
/* - Pins 10 to 15: EXTI15_10_IRQHandler                         */
/* 2. Update RADAR_EXTI_IRQn below to match the pin group           */
/* 3. Update RADAR_EXTI_PORT_SRC if shifting to a different PORT    */
/* ------------------------------------------------------------------ */
#define RADAR_GPIO_PORT     GPIOA
#define RADAR_GPIO_CLK_BIT  0U      /**< AHB1ENR bit: 0=GPIOA, 1=GPIOB, 2=GPIOC */
#define RADAR_EXTI_PORT_SRC   0U      /**< SYSCFG EXTICR code: 0=PA, 1=PB, 2=PC   */
#define RADAR_PIN           1U      /**< GPIO pin number (PA1) */
#define RADAR_EXTI_IRQn     EXTI1_IRQn

/* ------------------------------------------------------------------ */
/* Return Value Definitions                                            */
/* ------------------------------------------------------------------ */
#define RADAR_PRESENCE      1U      /**< Motion detected and validated */
#define RADAR_ABSENCE       0U      /**< No motion detected            */

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/** @brief Initializes GPIO and EXTI for the radar sensor. */
void    Radar_EXTI_Init(void);

/** @brief ISR callback. Call inside EXTI1_IRQHandler only. */
void    Radar_EXTI_Callback(void);

/**
 * @brief  Time-filtered presence detection.
 * @retval RADAR_PRESENCE or RADAR_ABSENCE.
 * @note   Poll every 10-50ms in STATE_SCANNING for accurate results.
 */
uint8_t Radar_Is_Detected(void);

/** @brief Resets all internal states. Call when entering STATE_IDLE. */
void    Radar_ClearPresence(void);

#endif /* RADAR_EXTI_H */
