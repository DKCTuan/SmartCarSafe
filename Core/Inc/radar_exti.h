/**
 * @file    radar_exti.h
 * @author  Mem 2
 * @brief   Driver cảm biến Radar RCWL-0516 qua EXTI, thanh ghi thuần.
 */
#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"
#include <stdint.h>

#define RADAR_PRESENCE   1U
#define RADAR_ABSENCE    0U

/* Biến tick hệ thống dùng chung toàn nhóm - định nghĩa ở main.c hoặc stm32f4xx_it.c */
extern volatile uint32_t g_tick_ms;

void    Radar_EXTI_Init(void);
void    Radar_EXTI_Callback(void);
uint8_t Radar_Is_Detected(void);
void    Radar_ClearPresence(void);

#endif /* RADAR_EXTI_H */
