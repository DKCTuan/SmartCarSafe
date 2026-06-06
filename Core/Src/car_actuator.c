/*
 * car_actuator.c
 *
 *
 */

#include "car_actuator.h"
#include <stdint.h>

/* Khởi tạo cấu hình bộ đếm Timer và các kênh PWM dùng cho Quạt và Servo */
void Car_Actuator_Init(void);

/* Điều khiển tốc độ quạt thông gió bên trong cabin dựa trên các mức (ví dụ: 0, 1, 2) */
void Actuator_Set_Fan_Speed(uint8_t speed_level);

/* Điều khiển góc quay của động cơ Servo để mô phỏng cơ chế tự động hạ kính (nhận vào góc 0-90) */
void Actuator_Set_Window_Position(uint8_t angle);
