#include "car_actuator.h"

/* Biến lưu trữ cấu hình phần cứng do main.c truyền vào */
static Car_Actuator_Config_t hw_config;

/* Kiem tra cau hinh chan co hop le hay khong.
 * Neu main.c khong khai bao servo that, port = 0 va driver bo qua TIM3_CH3.
 */
static uint8_t Is_Valid_Pin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    return ((GPIOx != (void *)0) && (pin < 16U)) ? 1U : 0U;
}

/* Hàm cấu hình một chân bất kỳ sang output đẩy-kéo (push-pull) */
static void Init_Output_Pin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    /* Bật clock tương ứng cho port */
    switch ((uint32_t)GPIOx) {
        case (uint32_t)GPIOA: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; break;
        case (uint32_t)GPIOB: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; break;
        case (uint32_t)GPIOC: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; break;
    }

    /* Tính toán vị trí bit */
    uint8_t bit_pos = pin << 1;

    /* Cấu hình moder: xóa 2 bit cũ và ghi 01 để đưa vào chế độ output */
    GPIOx->MODER &= ~(0b11 << bit_pos);
    GPIOx->MODER |=  (0b01 << bit_pos);

    /* Cấu hình otyper: ghi 0 để chọn kiểu xuất push-pull */
    GPIOx->OTYPER &= ~(0b1 << pin);

    /* Cấu hình ospeedr: ghi 10 để chọn tốc độ high speed */
    GPIOx->OSPEEDR &= ~(0b11 << bit_pos);
    GPIOx->OSPEEDR |=  (0b10 << bit_pos);

    /* Cấu hình pupdr: xóa 2 bit về 00 để chọn no pull-up/pull-down */
    GPIOx->PUPDR &= ~(0b11 << bit_pos);
}

/* Hàm cấu hình một chân bất kỳ sang alternate function (PWM) */
static void Init_AF_Pin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t af_num) {
    /* Bật clock tương ứng cho port */
    switch ((uint32_t)GPIOx) {
        case (uint32_t)GPIOA: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; break;
        case (uint32_t)GPIOB: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; break;
        case (uint32_t)GPIOC: RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; break;
    }

    /* Cấu hình moder: ghi 10 để chọn chế độ alternate function */
    uint8_t moder_pos = pin << 1;
    GPIOx->MODER &= ~(0b11 << moder_pos);
    GPIOx->MODER |=  (0b10 << moder_pos);

    /* Tính toán vị trí thanh ghi AFR bằng thuật toán chia mảng */
    uint8_t afr_index = pin / 8;     /* Chọn thanh ghi AFR[0] hoặc AFR[1] */
    uint8_t afr_pin   = pin % 8;     /* Tính số thứ tự chân tương đối (0-7) */
    uint8_t afr_pos   = afr_pin << 2; /* Mỗi chân chiếm 4 bit dịch */

    /* Xóa 4 bit cũ và nạp mã chức năng thay thế (af_num) tương ứng của timer */
    GPIOx->AFR[afr_index] &= ~(0b1111 << afr_pos);
    GPIOx->AFR[afr_index] |=  (af_num << afr_pos);
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
    /* Chi cau hinh servo neu project co chan servo that trong pinout. */
    if (Is_Valid_Pin(hw_config.servo.port, hw_config.servo.pin) != 0U) {
        Init_AF_Pin(hw_config.servo.port, hw_config.servo.pin, hw_config.servo.af_num);
    }

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
    if (Is_Valid_Pin(hw_config.servo.port, hw_config.servo.pin) != 0U) {
        TIM3->CCMR2 &= ~TIM_CCMR2_OC3M;
        TIM3->CCMR2 |= (6U << 4);
        TIM3->CCMR2 |= TIM_CCMR2_OC3PE;
        TIM3->CCER  |= TIM_CCER_CC3E;
    }

    /* Bật bộ đếm và nạp cấu hình shadow */
    TIM3->CR1 |= TIM_CR1_CEN;
    TIM3->EGR |= TIM_EGR_UG;

    /* Đặt trạng thái ban đầu an toàn (tắt quạt, đóng kính) */
    TIM3->CCR1 = 0;
    if (Is_Valid_Pin(hw_config.servo.port, hw_config.servo.pin) != 0U) {
        TIM3->CCR3 = 1000;
    }
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
    /* Khong lap servo thi ham nay khong lam gi, tranh ghi vao TIM3_CH3 vo nghia. */
    if (Is_Valid_Pin(hw_config.servo.port, hw_config.servo.pin) == 0U) {
        return;
    }

    /* Giới hạn góc quay tối đa để bảo vệ cơ cấu servo (0 - 90 độ) */
    if (angle > 90) {
        angle = 90;
    }

    /* Nội suy tuyến tính: 0 độ -> 1ms (1000), 90 độ -> 1.5ms (1500) */
    uint32_t pulse_width = 1000 + ((uint32_t)angle * 500) / 90;

    /* Nạp độ rộng xung vào kênh 3 của timer 3 */
    TIM3->CCR3 = pulse_width;
}
