#ifndef CAR_ACTUATOR_H
#define CAR_ACTUATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Dinh nghia cac chan dieu khien mach cau hinh TB6612 va quat */
#define MOTOR_PORT_A        GPIOA
#define MOTOR_PORT_B        GPIOB
#define MOTOR_PWMA_PIN      6   /* Chan PA6 tuong ung TIM3_CH1 */
#define MOTOR_AIN1_PIN      7   /* Chan PA7 cau hinh GPIO Output */
#define MOTOR_AIN2_PIN      0   /* Chan PB0 cau hinh GPIO Output */
#define MOTOR_STBY_PIN      1   /* Chan PB1 cau hinh GPIO Output */

/* Dinh nghia chan dieu khien dong co servo mo phong ha kinh */
/* Do bang chan cua nhom chua co servo, tam thoi dung chan PC8 tuong ung TIM3_CH3 de dung chung TIM3 */
#define SERVO_PORT          GPIOC
#define SERVO_PWM_PIN       8   /* Chan PC8 tuong ung TIM3_CH3 */

/* Khai bao cac ham giao tiep khoi chap hanh */
void Car_Actuator_Init(void);
void Actuator_Set_Fan_Speed(uint8_t speed_level);
void Actuator_Set_Window_Position(uint8_t angle);

#endif /* CAR_ACTUATOR_H */

