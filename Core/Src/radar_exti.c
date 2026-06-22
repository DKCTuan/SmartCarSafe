/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   Trình điều khiển cảm biến vi sóng RCWL-0516 tích hợp bộ lọc thời gian
 * và bộ đếm giữ trạng thái (hold timer). Lập trình trực tiếp qua thanh ghi
 * (Bare-metal) cho vi điều khiển STM32F411RE.
 * @date    2026
 */
#include "radar_exti.h"

/* ------------------------------------------------------------------ */
/* Timing Constants                                                    */
/* ------------------------------------------------------------------ */
#define RADAR_FILTER_MS   60U     /**< Thời gian lọc nhiễu: Tín hiệu phải giữ ở mức CAO liên tục >= 60ms để xác nhận là có chuyển động thật */
#define RADAR_HOLD_MS     3000U   /**< Thời gian giữ trạng thái: Tiếp tục báo CÓ NGƯỜI thêm 3 giây (3000ms) sau khi chân tín hiệu tụt xuống mức THẤP */

/* ------------------------------------------------------------------ */
/* Internal State Variables                                            */
/* ------------------------------------------------------------------ */
static volatile uint8_t  s_raw_trigger     = RADAR_ABSENCE;	/**< Cờ lưu trạng thái ngắt thô từ phần cứng (volatile vì bị thay đổi trong ngắt) */
static          uint8_t  s_validated_state = RADAR_ABSENCE;	/**< Trạng thái cuối cùng sau khi đã qua bộ lọc nhiễu */
static          uint8_t  s_filter_active   = 0U;	/**< Cờ báo hiệu bộ đếm thời gian lọc nhiễu đang chạy (1 = Đang chạy) */
static          uint32_t s_filter_start    = 0U;	/**< Mốc thời gian (SysTick) bắt đầu quá trình lọc nhiễu */
static          uint8_t  s_hold_active     = 0U;	/**< Cờ báo hiệu bộ đếm thời gian giữ trạng thái đang chạy */
static          uint32_t s_hold_start      = 0U;	/**< Mốc thời gian (SysTick) bắt đầu quá trình giữ trạng thái */

/* ------------------------------------------------------------------ */
/* Private: GPIO Configuration                                         */
/* ------------------------------------------------------------------ */
static void Radar_GPIO_Init(void)
{
	/* 1. Kích hoạt xung nhịp (Clock) cho cổng GPIO đang kết nối với Radar */
    RCC->AHB1ENR |= (1U << RADAR_GPIO_CLK_BIT);

    /* 2. Cấu hình chân ở chế độ Đầu vào (Input mode -> MODER = 00)
         * Xóa 2 bit cấu hình của chân tương ứng về 0. */
    RADAR_GPIO_PORT->MODER &= ~(3U << (RADAR_PIN * 2));

    /* 3. Cấu hình điện trở kéo xuống (Pull-Down -> PUPDR = 10)
         * Đảm bảo khi Radar không xuất tín hiệu, chân vi điều khiển không bị nhiễu nổi (floating). */
    RADAR_GPIO_PORT->PUPDR &= ~(3U << (RADAR_PIN * 2));
    RADAR_GPIO_PORT->PUPDR |=  (2U << (RADAR_PIN * 2));
}

/* ------------------------------------------------------------------ */
/* Public: Khởi Tạo Ngắt Ngoại Vi (EXTI) và Bộ Điều Khiển Ngắt (NVIC) */
/* ------------------------------------------------------------------ */
void Radar_EXTI_Init(void)
{
	/* Bước 1: Khởi tạo phần cứng chân GPIO */
	Radar_GPIO_Init();

	/* Bước 2: Kích hoạt xung nhịp cho bộ SYSCFG (System Configuration Controller)
	 * Bắt buộc phải bật để có thể ánh xạ đường ngắt EXTI. */
	RCC->APB2ENR |= (1U << 14);

	/* Bước 3: Ánh xạ đường ngắt EXTI tới cổng GPIO của Radar
	 * - Xóa 4 bit cấu hình cũ của chân tương ứng trong thanh ghi EXTICR.
	 * - Ghi mã nguồn của cổng mới vào (Ví dụ: 0=Port A, 1=Port B, 2=Port C). */
	SYSCFG->EXTICR[RADAR_PIN / 4] &= ~(15U << ((RADAR_PIN % 4) * 4));
	/* Write the actual port source code (0=PA, 1=PB, 2=PC) */
	SYSCFG->EXTICR[RADAR_PIN / 4] |=  (RADAR_EXTI_PORT_SRC << ((RADAR_PIN % 4) * 4));

	/* Step 4: Configure EXTI line */
	EXTI->IMR  |=  (1U << RADAR_PIN);  /* Unmask interrupt line       */
	EXTI->RTSR |=  (1U << RADAR_PIN);  /* Trigger on Rising edge only */
	EXTI->FTSR &= ~(1U << RADAR_PIN);  /* Disable Falling edge        */
	EXTI->PR    =  (1U << RADAR_PIN);  /* Clear any stale pending flag */

	/* Step 5: Configure NVIC */
	NVIC_SetPriority(RADAR_EXTI_IRQn, 2);
	NVIC_EnableIRQ(RADAR_EXTI_IRQn);
}

/* ------------------------------------------------------------------ */
/* Public: ISR Callback                                                */
/* ------------------------------------------------------------------ */
void Radar_EXTI_Callback(void)
{
    s_raw_trigger = RADAR_PRESENCE;
}

/* ------------------------------------------------------------------ */
/* Public: Time-Filtered Presence Detection                            */
/* ------------------------------------------------------------------ */
uint8_t Radar_Is_Detected(void)
{
    uint8_t pin_high = (RADAR_GPIO_PORT->IDR & (1U << RADAR_PIN)) ? 1U : 0U;

    /* CASE 1: Interrupt fired AND pin is physically HIGH */
    if ((s_raw_trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        s_hold_active = 0U;  /* Cancel hold timer if motion resumes */

        if (s_filter_active == 0U) {
            s_filter_start  = SysTimer_GetTick();
            s_filter_active = 1U;
        }

        if ((SysTimer_GetTick() - s_filter_start) >= RADAR_FILTER_MS) {
            s_validated_state = RADAR_PRESENCE;
        }
    }
    /* CASE 2: Pin LOW or no raw trigger */
    else
    {
        s_filter_active = 0U;
        s_raw_trigger   = RADAR_ABSENCE;

        /*
         * HOLD TIMER: RCWL-0516 auto-pulls OUT pin LOW after ~2-3s
         * even if a person remains present. Hold PRESENCE state for
         * RADAR_HOLD_MS before declaring absence.
         */
        if (s_validated_state == RADAR_PRESENCE) {
            if (s_hold_active == 0U) {
                s_hold_start  = SysTimer_GetTick();
                s_hold_active = 1U;
            }
            if ((SysTimer_GetTick() - s_hold_start) >= RADAR_HOLD_MS) {
                s_validated_state = RADAR_ABSENCE;
                s_hold_active     = 0U;
            }
        }
    }
    return s_validated_state;
}

/* ------------------------------------------------------------------ */
/* Public: Full State Reset                                            */
/* ------------------------------------------------------------------ */
void Radar_ClearPresence(void)
{
    s_raw_trigger     = RADAR_ABSENCE;
    s_validated_state = RADAR_ABSENCE;
    s_filter_active   = 0U;
    s_hold_active     = 0U;
}
