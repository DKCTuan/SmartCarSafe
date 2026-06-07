/**
 * @file    radar_exti.h
 * @author  Mem 2
 * @brief   Driver cảm biến Radar RCWL-0516 qua EXTI, dùng thanh ghi thuần.
 */
#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"
#include <stdint.h>

#define RADAR_PRESENCE   1U
#define RADAR_ABSENCE    0U

void Radar_EXTI_Init(void);
uint8_t Radar_GetPresence(void);
void Radar_ClearPresence(void);
void Radar_EXTI_Callback(void);

#endif /* RADAR_EXTI_H */
