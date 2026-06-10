#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Cấu trúc lưu trữ chân GPIO xuất tín hiệu bình thường (cho mạch cầu H) */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} Actuator_Gpio_Config_t;

/* Cấu trúc lưu trữ chân băm xung PWM (cần khớp với Timer 3) */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
    uint8_t       af_num; /* Mã alternate function (ví dụ: 2 cho TIM3) */
} Actuator_Pwm_Config_t;

/* Cấu trúc tổng hợp toàn bộ phần cứng của module */
typedef struct {
    Actuator_Gpio_Config_t ain1;  /* Chân chiều quay 1 của quạt */
    Actuator_Gpio_Config_t ain2;  /* Chân chiều quay 2 của quạt */
    Actuator_Gpio_Config_t stby;  /* Chân standby kích hoạt TB6612 */
    Actuator_Pwm_Config_t  pwma;  /* Chân băm xung tốc độ quạt (TIM3_CH1) */
    Actuator_Pwm_Config_t  servo; /* Chân băm xung góc kính xe (TIM3_CH3) */
} Car_Actuator_Config_t;

/* Hàm khởi tạo cấu hình phần cứng (nhận mảng cấu hình từ main.c) */
void Car_Actuator_Init(Car_Actuator_Config_t *config);

/* Hàm điều khiển tốc độ quạt gió (0: Tắt, 1: Vừa, 2: Mạnh) */
void Actuator_Set_Fan_Speed(uint8_t speed_level);

/* Hàm điều khiển vị trí cửa kính bằng servo (0 - 90 độ) */
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */

