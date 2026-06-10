#include "car_actuator.h"

/* Biến lưu trữ cấu hình phần cứng do main.c truyền vào */
static Car_Actuator_Config_t hw_config;

/* Hàm cấu hình một chân bất kỳ sang output đẩy-kéo (push-pull) */
static void Init_Output_Pin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    /* Bật clock */
    if      (GPIOx == GPIOA) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; }
    else if (GPIOx == GPIOB) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; }
    else if (GPIOx == GPIOC) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; }

    /* Cấu hình moder: chuyển về 01 (output) */
    GPIOx->MODER &= ~(3U << (pin * 2));
    GPIOx->MODER |=  (1U << (pin * 2));

    /* Cấu hình otyper: push-pull (0) */
    GPIOx->OTYPER &= ~(1U << pin);

    /* Cấu hình ospeedr: high speed (10) */
    GPIOx->OSPEEDR &= ~(3U << (pin * 2));
    GPIOx->OSPEEDR |=  (2U << (pin * 2));

    /* Cấu hình pupdr: no pull (00) */
    GPIOx->PUPDR &= ~(3U << (pin * 2));
}

/* Hàm cấu hình một chân bất kỳ sang alternate function (PWM) */
static void Init_AF_Pin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t af_num) {
    /* Bật clock */
    if      (GPIOx == GPIOA) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; }
    else if (GPIOx == GPIOB) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; }
    else if (GPIOx == GPIOC) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; }

    /* Cấu hình moder: chuyển về 10 (alternate function) */
    GPIOx->MODER &= ~(3U << (pin * 2));
    GPIOx->MODER |=  (2U << (pin * 2));

    /* Cấu hình AFR (chọn đúng luồng tín hiệu timer) */
    if (pin < 8) {
        GPIOx->AFR[0] &= ~(15U << (pin * 4));
        GPIOx->AFR[0] |=  (af_num << (pin * 4));
    } else {
        uint8_t pin_high = pin - 8;
        GPIOx->AFR[1] &= ~(15U << (pin_high * 4));
        GPIOx->AFR[1] |=  (af_num << (pin_high * 4));
    }
}

/* Hàm khởi tạo cấu hình timer và các chân liên quan dựa trên cấu hình từ main.c */
void Car_Actuator_Init(Car_Actuator_Config_t *config) {
    /* 1. Lưu trữ cấu hình vào biến nội bộ để dùng cho các hàm sau */
    hw_config = *config;

    /* 2. Cấu hình các chân điều khiển động cơ (output bình thường) */
    Init_Output_Pin(hw_config.ain1.port, hw_config.ain1.pin);
    Init_Output_Pin(hw_config.ain2.port, hw_config.ain2.pin);
    Init_Output_Pin(hw_config.stby.port, hw_config.stby.pin);

    /* 3. Cấu hình các chân băm xung PWM (alternate function) */
    Init_AF_Pin(hw_config.pwma.port, hw_config.pwma.pin, hw_config.pwma.af_num);
    Init_AF_Pin(hw_config.servo.port, hw_config.servo.pin, hw_config.servo.af_num);

    /* 4. Cấu hình bộ định thời timer 3 tạo xung 50Hz (chu kỳ 20ms) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = 16 - 1;       /* Chia tần 16MHz xuống 1MHz (1us/tick) */
    TIM3->ARR = 20000 - 1;    /* Đếm đến 20000 -> 20ms */

    /* Cấu hình kênh 1 (PWM1 mode) cho quạt gió */
    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM3->CCMR1 |= (6U << 4); /* Mode 110: PWM mode 1 */
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM3->CCER  |= TIM_CCER_CC1E;

    /* Cấu hình kênh 3 (PWM1 mode) cho servo */
    TIM3->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM3->CCMR2 |= (6U << 4);
    TIM3->CCMR2 |= TIM_CCMR2_OC3PE;
    TIM3->CCER  |= TIM_CCER_CC3E;

    /* Bật bộ đếm và nạp cấu hình shadow */
    TIM3->CR1 |= TIM_CR1_CEN;
    TIM3->EGR |= TIM_EGR_UG;

    /* Đặt trạng thái ban đầu an toàn (tắt quạt, đóng kính) */
    TIM3->CCR1 = 0;
    TIM3->CCR3 = 1000;
}

/* Hàm điều khiển tốc độ quạt gió (0: Tắt, 1: Vừa, 2: Mạnh) */
void Actuator_Set_Fan_Speed(uint8_t speed_level) {
    /* Trích xuất mặt nạ bit của các chân điều khiển */
    uint32_t ain1_mask = (1U << hw_config.ain1.pin);
    uint32_t ain2_mask = (1U << hw_config.ain2.pin);
    uint32_t stby_mask = (1U << hw_config.stby.pin);

    switch (speed_level) {
        case 0: /* Tắt hoàn toàn */
            hw_config.stby.port->ODR &= ~stby_mask;
            hw_config.ain1.port->ODR &= ~ain1_mask;
            hw_config.ain2.port->ODR &= ~ain2_mask;
            TIM3->CCR1 = 0;
            break;

        case 1: /* Tốc độ vừa (50%), quay thuận */
            hw_config.stby.port->ODR |=  stby_mask; /* Kích hoạt TB6612 */
            hw_config.ain1.port->ODR |=  ain1_mask; /* AIN1 = 1 */
            hw_config.ain2.port->ODR &= ~ain2_mask; /* AIN2 = 0 */
            TIM3->CCR1 = 10000; /* Duty cycle 50% */
            break;

        case 2: /* Tốc độ tối đa (100%), quay thuận */
            hw_config.stby.port->ODR |=  stby_mask;
            hw_config.ain1.port->ODR |=  ain1_mask;
            hw_config.ain2.port->ODR &= ~ain2_mask;
            TIM3->CCR1 = 20000; /* Duty cycle 100% */
            break;

        default:
            break;
    }
}

/* Hàm điều khiển vị trí cửa kính bằng servo (0 - 90 độ) */
void Actuator_Set_Window_Position(uint8_t angle) {
    /* Giới hạn góc quay tối đa để bảo vệ cơ cấu servo (0 - 90 độ) */
    if (angle > 90) {
        angle = 90;
    }

    /* Nội suy tuyến tính: 0 độ -> 1ms (1000), 90 độ -> 1.5ms (1500) */
    uint32_t pulse_width = 1000 + ((uint32_t)angle * 500) / 90;

    /* Nạp độ rộng xung vào kênh 3 của timer 3 */
    TIM3->CCR3 = pulse_width;
}
