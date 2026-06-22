/**
 * @file    main.c
 * @author  Mem 2
 * @brief   Chương trình test độc lập module Radar RCWL-0516 (Bare-metal)
 */

#include "stm32f4xx.h"
#include "sys_timer.h"
#include "radar_exti.h"

/* ------------------------------------------------------------------ */
/* Định nghĩa phần cứng cho LED test (LD2 trên mạch Nucleo F411RE)     */
/* ------------------------------------------------------------------ */
#define LED_PORT GPIOA
#define LED_PIN  5U

/**
 * @brief Khởi tạo chân PA5 ở chế độ Output Push-Pull để điều khiển LED
 */
void LED_Init(void) {
    /* 1. Bật xung nhịp cho cổng GPIOA */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Cấu hình chân PA5 là Output (MODER = 01) */
    LED_PORT->MODER &= ~(3U << (LED_PIN * 2));
    LED_PORT->MODER |=  (1U << (LED_PIN * 2));

    /* 3. Tắt LED lúc mới khởi động */
    LED_PORT->ODR &= ~(1U << LED_PIN);
}

/**
 * @brief Bật hoặc tắt LED
 * @param state: 1 để bật LED, 0 để tắt LED
 */
void LED_Write(uint8_t state) {
    if (state == 1U) {
        LED_PORT->ODR |= (1U << LED_PIN);
    } else {
        LED_PORT->ODR &= ~(1U << LED_PIN);
    }
}

/* ------------------------------------------------------------------ */
/* Chương trình chính                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    /* * Bước 1: Khởi tạo SysTick Timer
     * Vi điều khiển STM32F411RE mặc định chạy xung nội HSI ở tần số 16MHz.
     * Cấp giá trị 16000000 để tạo ra ngắt SysTick chuẩn xác 1ms.
     */
    SysTimer_Init(16000000U);

    /* Bước 2: Khởi tạo các ngoại vi */
    LED_Init();
    Radar_EXTI_Init();

    /* Bước 3: Vòng lặp vô tận (Super-loop) */
    while (1) {
        /* Liên tục gọi hàm kiểm tra Radar (đã bao gồm lọc nhiễu và đếm thời gian) */
        uint8_t radar_status = Radar_Is_Detected();

        /* Điều khiển LED dựa trên trạng thái của Radar */
        if (radar_status == RADAR_PRESENCE) {
            LED_Write(1U);  /* Sáng đèn LED LD2 trên board */
        } else {
            LED_Write(0U);  /* Tắt đèn LED LD2 */
        }
    }
}
