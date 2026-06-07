/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   Cài đặt driver RCWL-0516 bằng thanh ghi thuần.
 */
#include "radar_exti.h"

static volatile uint8_t s_radar_presence_flag = RADAR_ABSENCE;

void Radar_EXTI_Init(void){
    /* 1. CẤP CLOCK */
    RCC->AHB1ENR |= (1U << 0);   /* Bật GPIOA */
    RCC->APB2ENR |= (1U << 14);  /* Bật SYSCFG */

    /* 2. CẤU HÌNH PA0 = INPUT PULL-DOWN */
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |=  (2U << (0 * 2));  /* Pull-Down */

    /* 3. CẤU HÌNH EXTI LINE 0 */
    SYSCFG->EXTICR[0] &= ~(15U << (0 * 4));
    EXTI->IMR  |=  (1U << 0);
    EXTI->RTSR |=  (1U << 0);    /* Kích hoạt cạnh lên */
    EXTI->FTSR &= ~(1U << 0);
    EXTI->PR = (1U << 0);        /* Xóa cờ bẩn ban đầu */

    /* 4. CẤU HÌNH NVIC */
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void Radar_EXTI_Callback(void){
    s_radar_presence_flag = RADAR_PRESENCE;
}

uint8_t Radar_GetPresence(void){
    return s_radar_presence_flag;
}

void Radar_ClearPresence(void){
    s_radar_presence_flag = RADAR_ABSENCE;
}
