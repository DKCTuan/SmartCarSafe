/*
 * car_actuator.h
 *
 *  Created on: Jun 3, 2026
 *      Author: DINH KIEU CONG TUAN
 */

#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"

/* Hàm khởi tạo GPIO Input cho công tắc và Timer PWM cho Servo/Quạt */
void Car_Actuator_Init(void);

/* Trọng tự viết hàm đọc trạng thái xe (có xử lý Debounce) ở đây */
uint8_t Car_Get_ACC_Status(void);
uint8_t Car_Get_Lock_Status(void);

/* Trọng tự viết hàm điều khiển PWM tốc độ quạt (0, 1, 2) và góc quay Servo (0, 90) */
void Actuator_Set_Fan_Speed(uint8_t speed_level);
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */
