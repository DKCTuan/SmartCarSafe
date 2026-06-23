#include "stm32f4xx.h"
#include "stm32f4xx_hal.h" /* Bắt buộc phải có để gọi HAL_Init */
#include "sys_timer.h"
#include "car_state.h"

void Test_LED_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER &= ~(3U << (0 * 2));
    GPIOB->MODER |=  (1U << (0 * 2));
}

int main(void) {
    /* 1. Kích hoạt toàn bộ hệ thống lõi của thư viện HAL (Bao gồm cả việc bật Timer) */
    HAL_Init();

    /* 2. Hàm của Mem 2 lúc này chạy qua cho có lệ (vì HAL_Init đã lo rồi) */
    SysTimer_Init(16000000);

    /* 3. Khởi tạo chân nút bấm (PC1) và chân LED (PB0) */
    Car_Pin_Config_t test_pins[CAR_SIGNAL_MAX] = {
        {GPIOC, 0}, {GPIOC, 1}, {GPIOC, 2}
    };
    Car_State_Init(test_pins);
    Test_LED_Init();

    while (1) {
        uint8_t door_state = Car_Get_System_Status(CAR_DOOR_SIGNAL);
        if (door_state == 1) {
            GPIOB->ODR |= (1U << 0);
        } else {
            GPIOB->ODR &= ~(1U << 0);
        }
    }
}
