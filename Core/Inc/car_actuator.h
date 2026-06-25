#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/**
 * @brief  Cấu trúc lưu trữ cấu hình chân GPIO xuất tín hiệu bình thường (cho mạch cầu H)
 */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} Actuator_Gpio_Config_t;

/**
 * @brief  Cấu trúc lưu trữ cấu hình chân băm xung PWM (cần khớp với cấu hình Timer)
 */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
    uint8_t       af_num; /* Mã alternate function (ví dụ: 2 cho TIM3) */
} Actuator_Pwm_Config_t;

/**
 * @brief  Cấu trúc tổng hợp toàn bộ phần cứng của module cơ cấu chấp hành
 */
typedef struct {
    Actuator_Gpio_Config_t ain1;  /* Chân chiều quay 1 của quạt */
    Actuator_Gpio_Config_t ain2;  /* Chân chiều quay 2 của quạt */
    Actuator_Gpio_Config_t stby;  /* Chân standby kích hoạt TB6612 */
    Actuator_Pwm_Config_t  pwma;  /* Chân băm xung tốc độ quạt (TIM3_CH1) */
    Actuator_Pwm_Config_t  servo; /* Chân băm xung góc kính xe (TIM3_CH3) */

    Actuator_Gpio_Config_t led_acc;  /* Chân LED báo ACC */
    Actuator_Gpio_Config_t led_door; /* Chân LED báo DOOR */
    Actuator_Gpio_Config_t led_lock; /* Chân LED báo LOCK */
} Car_Actuator_Config_t;

/**
 * @brief  Điều khiển trạng thái bật/tắt của 3 LED báo hiệu nút bấm
 * @param  acc_on: Trạng thái của tín hiệu ACC (1: Bật LED, 0: Tắt LED)
 * @param  door_on: Trạng thái của tín hiệu Cửa (1: Bật LED, 0: Tắt LED)
 * @param  lock_on: Trạng thái của tín hiệu Khóa (1: Bật LED, 0: Tắt LED)
 */
void Actuator_Set_StatusLED(uint8_t acc_on, uint8_t door_on, uint8_t lock_on);

/**
 * @brief  Khởi tạo cấu hình phần cứng cho các cơ cấu chấp hành
 * @param  config: Con trỏ tới cấu trúc lưu trữ cài đặt chân phần cứng truyền từ main.c
 */
void Car_Actuator_Init(Car_Actuator_Config_t *config);

/**
 * @brief  Điều khiển tốc độ quạt gió thông qua mạch công suất TB6612FNG
 * @param  speed_level: Mức tốc độ mong muốn (0: Tắt, 1: Vừa, 2: Mạnh)
 */
void Actuator_Set_Fan_Speed(uint8_t speed_level);

/**
 * @brief  Điều khiển vị trí góc mở của cửa kính xe bằng động cơ Servo
 * @param  angle: Góc quay mong muốn của Servo (giới hạn an toàn từ 0 - 90 độ)
 */
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */
