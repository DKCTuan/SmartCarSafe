/*
 * radar_exti.h
 *
 *  Created on: Jun 3, 2026
 *      Author: DINH KIEU CONG TUAN
 */

#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"

/* Hàm khởi tạo ngắt ngoài EXTI cho Radar */
void Radar_EXTI_Init(void);

/* Hàm trả về 1 nếu phát hiện chuyển động sau bộ lọc thời gian, 0 nếu không có */
uint8_t Radar_Is_Detected(void);

#endif /* RADAR_EXTI_H */
