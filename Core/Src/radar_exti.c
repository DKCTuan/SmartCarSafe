/**
 * @file    radar_exti.c
 * @brief   Sửa lỗi logic bộ lọc thời gian và cấu hình ngắt cạnh kép cho Radar RCWL-0516.
 * @author  Tuan & Trong (VNU-UET)
 */
#include "radar_exti.h"
#include "main.h"

#define RADAR_FILTER_MS   30U
#define RADAR_HOLD_MS     3000U

static volatile uint8_t  s_raw_trigger     = RADAR_ABSENCE;
static          uint8_t  s_validated_state = RADAR_ABSENCE;
static          uint8_t  s_hold_active     = 0U;
static          uint32_t s_hold_start      = 0U;

static void Radar_GPIO_Init(void)
{
    RCC->AHB1ENR |= (1U << RADAR_GPIO_CLK_BIT);
    RADAR_GPIO_PORT->MODER &= ~(3U << (RADAR_PIN * 2));

    RADAR_GPIO_PORT->PUPDR &= ~(3U << (RADAR_PIN * 2));
}

void Radar_EXTI_Init(void)
{
    Radar_GPIO_Init();
    RCC->APB2ENR |= (1U << 14); /* Bật SYSCFG clock */

    SYSCFG->EXTICR[RADAR_PIN / 4] &= ~(15U << ((RADAR_PIN % 4) * 4));

    /* SỬA LỖI 2: Cấu hình bắt ngắt CẠNH KÉP (Cả lên và xuống) để bám sát phần cứng */
    EXTI->IMR  |=  (1U << RADAR_PIN);
    EXTI->RTSR |=  (1U << RADAR_PIN);  /* Kích hoạt cạnh lên */
    EXTI->FTSR |=  (1U << RADAR_PIN);  /* Kích hoạt cạnh xuống */
    EXTI->PR    =  (1U << RADAR_PIN);

    NVIC_SetPriority(RADAR_EXTI_IRQn, 2);
    NVIC_EnableIRQ(RADAR_EXTI_IRQn);
}

void Radar_EXTI_Callback(void)
{
    if ((RADAR_GPIO_PORT->IDR & (1U << RADAR_PIN)) != 0U)
    {
        s_raw_trigger = RADAR_PRESENCE;
        /* Không set ABSENCE ở đây nữa */
    }
    /* Cạnh xuống: bỏ qua trong ISR, để hold timer tự xử lý */
}

// radar_exti.c - Radar_Is_Detected() viết lại hoàn chỉnh

uint8_t Radar_Is_Detected(void)
{
    uint32_t now = SysTimer_GetTick();

    /* Đọc an toàn biến ngắt */
    __disable_irq();
    uint8_t trigger = s_raw_trigger;
    __enable_irq();

    /* Đọc trạng thái vật lý thực tế của chân PA1 */
    uint8_t pin_high = (RADAR_GPIO_PORT->IDR & (1U << RADAR_PIN)) ? 1U : 0U;

    /* Trường hợp 1: Có tín hiệu kích hoạt VÀ chân vật lý vẫn đang giữ mức CAO */
    if ((trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        s_hold_active = 0U; /* Reset bộ đếm giữ chân */

        /* Nếu chưa kích hoạt bộ lọc, bắt đầu ghi nhận mốc thời gian */
        if (s_hold_start == 0U)
        {
            s_hold_start = now;
        }

        /* Tín hiệu phải duy trì liên tục vượt qua ngưỡng FILTER (30ms) mới công nhận */
        if ((now - s_hold_start) >= RADAR_FILTER_MS)
        {
            s_validated_state = RADAR_PRESENCE;
        }
    }
    /* Trường hợp 2: Chân vật lý đã tụt về mức THẤP (Hết chuyển động) */
    else
    {
        s_hold_start = 0U; /* Xoá mốc thời gian bộ lọc */

        __disable_irq();
        s_raw_trigger = RADAR_ABSENCE; /* Xoá cờ ngắt cũ */
        __enable_irq();

        /* Kích hoạt bộ đếm lùi giữ trạng thái chống chập chờn */
        if (s_validated_state == RADAR_PRESENCE)
        {
            if (s_hold_active == 0U)
            {
                s_hold_start  = now;
                s_hold_active = 1U;
            }

            if ((now - s_hold_start) >= RADAR_HOLD_MS)
            {
                s_validated_state = RADAR_ABSENCE;
                s_hold_active     = 0U;
            }
        }
    }

    return s_validated_state;
}

void Radar_ClearPresence(void)
{
    s_raw_trigger     = RADAR_ABSENCE;
    s_validated_state = RADAR_ABSENCE;
    s_hold_active     = 0U;
}
