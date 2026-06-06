/*
 * car_actuator.h
 *
 *
 */

#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Định nghĩa chân đầu ra (PWM cho quạt & servo) */
#define PWM_PORT            GPIOA
/* PA6 (TIM3_CH1) */
#define SERVO_PIN           6
/* PA7 (TIM3_CH2) */
#define FAN_PIN             7

/* Cấu hình bộ định thời gian (Timer) trên STM32 để xuất ra tín hiệu băm xung PWM. */
void Car_Actuator_Init(void);

/* Điều khiển bật/tắt và thay đổi tốc độ quạt thông gió trong xe. */
void Actuator_Set_Fan_Speed(uint8_t speed_level);

/* Điều khiển động cơ Servo quay đến một góc nhất định
 * để mô phỏng việc hạ kính xe xuống cho thoáng khí. */
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */

