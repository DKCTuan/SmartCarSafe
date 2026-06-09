/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   Driver RCWL-0516 với bộ lọc thời gian non-blocking và chống nhiễu.
 */
#include "radar_exti.h"

#define RADAR_FILTER_MS     60U    /* Tín hiệu HIGH phải duy trì >= 60ms */
#define RADAR_HOLD_MS     3000U    /* Giữ kết quả PRESENCE thêm 3s sau khi chân xuống LOW */

static volatile uint8_t  s_raw_trigger       = RADAR_ABSENCE;
static          uint8_t  s_validated_state   = RADAR_ABSENCE;

static          uint8_t  s_filter_active     = 0U; /* Cờ đánh dấu đang trong quá trình lọc */
static          uint32_t s_filter_start      = 0U;

static          uint8_t  s_hold_active       = 0U; /* Cờ đánh dấu đang trong quá trình giữ mạch */
static          uint32_t s_hold_start        = 0U;

void Radar_EXTI_Init(void){
    /* 1. CẤP CLOCK */
    RCC->AHB1ENR |= (1U << 0);    /* Bật GPIOA */
    RCC->APB2ENR |= (1U << 14);   /* Bật SYSCFG */

    /* 2. CẤU HÌNH PA0 = INPUT PULL-DOWN */
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->PUPDR &= ~(3U << (0 * 2));  /* Xóa 2 bit cũ trước */
    GPIOA->PUPDR |=  (2U << (0 * 2));  /* Ghi 10 (Pull-Down) */

    /* 3. CẤU HÌNH EXTI LINE 0 */
    SYSCFG->EXTICR[0] &= ~(15U << (0 * 4)); /* Chọn PA0 làm nguồn ngắt cho EXTI0 */
    EXTI->IMR  |=  (1U << 0);     /* Mở ngắt hàng rào EXTI0 */
    EXTI->RTSR |=  (1U << 0);     /* Kích hoạt ngắt cạnh lên (Rising Edge) */
    EXTI->FTSR &= ~(1U << 0);     /* Tắt ngắt cạnh xuống */
    EXTI->PR    =  (1U << 0);     /* Clear cờ bẩn ban đầu */

    /* 4. CẤU HÌNH NVIC */
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void Radar_EXTI_Callback(void){
    s_raw_trigger = RADAR_PRESENCE;
}

uint8_t Radar_Is_Detected(void){
    uint8_t pin_high = (GPIOA->IDR & (1U << 0)) ? 1U : 0U;

    // TRƯỜNG HỢP 1: Có tín hiệu ngắt thô và chân thực sự đang HIGH
    if ((s_raw_trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        s_hold_active = 0U; /* Bẻ gãy quá trình đếm ngược HOLD nếu có người xuất hiện lại */

        if (s_filter_active == 0U) {
            s_filter_start  = g_tick_ms;
            s_filter_active = 1U; /* Bật cờ bắt đầu lọc */
        }

        /* Toán tử trừ không sợ tràn số (overflow) của biến uint32_t */
        if ((g_tick_ms - s_filter_start) >= RADAR_FILTER_MS) {
            s_validated_state = RADAR_PRESENCE;
        }
    }
    // TRƯỜNG HỢP 2: Chân sụt xuống mức LOW hoặc hết chuyển động thô
    else
    {
        s_filter_active = 0U; /* Reset bộ lọc thời gian */
        s_raw_trigger   = RADAR_ABSENCE;

        /* Nếu hệ thống đang ở trạng thái CÓ NGƯỜI, bắt đầu kích hoạt bộ giữ mạch 3 giây */
        if (s_validated_state == RADAR_PRESENCE) {
            if (s_hold_active == 0U) {
                s_hold_start  = g_tick_ms;
                s_hold_active = 1U;
            }

            if ((g_tick_ms - s_hold_start) >= RADAR_HOLD_MS) {
                s_validated_state = RADAR_ABSENCE;
                s_hold_active     = 0U;
            }
        }
    }
    return s_validated_state;
}

void Radar_ClearPresence(void){
    s_raw_trigger     = RADAR_ABSENCE;
    s_validated_state = RADAR_ABSENCE;
    s_filter_active   = 0U;
    s_hold_active     = 0U;
}
