#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Định nghĩa các chân điều khiển mạch cấu hình TB6612 và quạt */
#define MOTOR_PORT_A        GPIOA
#define MOTOR_PORT_B        GPIOB
#define MOTOR_PWMA_PIN      6   /* Chân PA6 tương ứng TIM3_CH1 */
#define MOTOR_AIN1_PIN      7   /* Chân PA7 cấu hình GPIO Output */
#define MOTOR_AIN2_PIN      0   /* Chân PB0 cấu hình GPIO Output */
#define MOTOR_STBY_PIN      1   /* Chân PB1 cấu hình GPIO Output */

/* Định nghĩa chân điều khiển động cơ servo mô phỏng hạ kính
 * Dùng chân PC8 tương ứng TIM3_CH3 để dùng chung TIM3 */
#define SERVO_PORT          GPIOC
#define SERVO_PWM_PIN       8   /* Chân PC8 tương ứng TIM3_CH3 */

/* Khai báo các hàm giao tiếp khối chấp hành */
void Car_Actuator_Init(void);
void Actuator_Set_Fan_Speed(uint8_t speed_level);
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */

