/**
 * @file    main.c
 * @author  Mem 2
 * @brief   Test RCWL-0516 Radar sensor with LED (PA5) and SysTick.
 * @date    2026
 */

#include "stm32f4xx.h"          /* thanh ghi */
#include "radar_exti.h"         /* driver radar */
#include "sys_timer.h"          /* system tick 1ms */

/* ------------------------------------------------------------------ */
/* Định nghĩa LED test (PA5 - LED xanh trên Nucleo)                    */
/* ------------------------------------------------------------------ */
#define LED_GPIO_PORT      GPIOA
#define LED_PIN            5U
#define LED_GPIO_CLK_BIT   0U      /* bit 0 trong RCC->AHB1ENR cho GPIOA */

/* ------------------------------------------------------------------ */
/* Hàm khởi tạo LED                                                    */
/* ------------------------------------------------------------------ */
static void LED_Init(void)
{
    /* Bật clock cho GPIOA */
    RCC->AHB1ENR |= (1U << LED_GPIO_CLK_BIT);

    /* Cấu hình PA5 là output push-pull, tốc độ cao, không kéo */
    LED_GPIO_PORT->MODER   &= ~(3U << (LED_PIN * 2));
    LED_GPIO_PORT->MODER   |=  (1U << (LED_PIN * 2));   /* 01 = output */

    LED_GPIO_PORT->OTYPER  &= ~(1U << LED_PIN);         /* push-pull */
    LED_GPIO_PORT->OSPEEDR &= ~(3U << (LED_PIN * 2));
    LED_GPIO_PORT->OSPEEDR |=  (2U << (LED_PIN * 2));   /* high speed */
    LED_GPIO_PORT->PUPDR   &= ~(3U << (LED_PIN * 2));   /* no pull */
}

static inline void LED_Set(uint8_t state)
{
    if (state)
        LED_GPIO_PORT->ODR |= (1U << LED_PIN);
    else
        LED_GPIO_PORT->ODR &= ~(1U << LED_PIN);
}

/* ------------------------------------------------------------------ */
/* Delay thô (không chính xác, chỉ để giảm tần suất kiểm tra)          */
/* ------------------------------------------------------------------ */
static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 10000; i++) {
        __NOP();
    }
}

/* ------------------------------------------------------------------ */
/* Chương trình chính                                                  */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* 1. Xác định nguồn clock và tần số CPU thực tế                     */
    uint8_t clock_source = (RCC->CFGR >> 2) & 0x03;
    uint32_t cpu_freq;

    switch(clock_source) {
        case 0: /* HSI */
            cpu_freq = 16000000;
            break;
        case 1: /* HSE (giả định thạch anh 8MHz) */
            cpu_freq = 8000000;
            break;
        case 2: /* PLL – ưu tiên dùng SystemCoreClock nếu có */
#ifdef SystemCoreClock
            cpu_freq = SystemCoreClock;
#else
            /* Nếu không có, cần tính từ RCC->PLLCFGR, nhưng tạm mặc định 84MHz */
            cpu_freq = 84000000;
#endif
            break;
        default:
            cpu_freq = 16000000;
            break;
    }

    /* 2. Khởi tạo LED (để có thể quan sát trạng thái ngay) */
    LED_Init();

    /* 3. Khởi tạo SysTick với tần số đã xác định (đảm bảo tick 1ms) */
    SysTimer_Init(cpu_freq);

    /* 4. Khởi tạo radar (cấu hình PA1, EXTI1, NVIC) */
    Radar_EXTI_Init();

    /* 5. Vòng lặp chính */
    while (1)
    {
        /* Kiểm tra trạng thái hiện diện (có bộ lọc và hold timer) */
        if (Radar_Is_Detected() == RADAR_PRESENCE) {
            LED_Set(1);   /* Bật LED khi phát hiện chuyển động */
        } else {
            LED_Set(0);   /* Tắt LED khi không có chuyển động */
        }

        /* Delay nhỏ để giảm tải CPU, tần suất kiểm tra khoảng 100Hz */
        delay_ms(10);
    }
}
