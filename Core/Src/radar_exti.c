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

	/* Bước 4: Cấu hình thanh ghi của đường ngắt EXTI */
	EXTI->IMR  |=  (1U << RADAR_PIN);  /* Mở mặt nạ ngắt (Unmask) cho phép đường ngắt này hoạt động */
	EXTI->RTSR |=  (1U << RADAR_PIN);  /* Cấu hình kích hoạt ngắt khi có sườn lên (Rising edge - từ 0V lên 3.3V) */
	EXTI->FTSR &= ~(1U << RADAR_PIN);  /* Vô hiệu hóa kích hoạt ngắt khi có sườn xuống (Falling edge) */
	EXTI->PR    =  (1U << RADAR_PIN);  /* Xóa các cờ ngắt cũ có thể còn tồn đọng trước khi hệ thống chạy */

	/* Bước 5: Cấu hình Bộ Điều Khiển Vectơ Ngắt Đa Chiều Lồng Nhau (NVIC) */
	NVIC_SetPriority(RADAR_EXTI_IRQn, 2); /* Đặt mức độ ưu tiên ngắt là 2 (Mức độ vừa phải) */
	NVIC_EnableIRQ(RADAR_EXTI_IRQn);	/* Cho phép kênh ngắt này được kích hoạt trên lõi vi xử lý ARM */
}

/* ------------------------------------------------------------------ */
/* Public: Hàm Xử Lý Gọi Ngược Từ Ngắt (ISR Callback)                                               */
/* ------------------------------------------------------------------ */
void Radar_EXTI_Callback(void)
{
	/* Ghi nhận cờ báo hiệu thô là đã có sự kiện kích hoạt sườn lên từ Radar */
    s_raw_trigger = RADAR_PRESENCE;
}

/* ------------------------------------------------------------------ */
/*Public: Thuật Toán Xác Nhận Chuyển Động (Lọc Nhiễu Thời Gian)                        */
/* ------------------------------------------------------------------ */
uint8_t Radar_Is_Detected(void)
{
	/* Đọc trạng thái logic vật lý thực tế của chân GPIO tại thời điểm hiện tại */
    uint8_t pin_high = (RADAR_GPIO_PORT->IDR & (1U << RADAR_PIN)) ? 1U : 0U;

    /* TRƯỜNG HỢP 1: Cờ ngắt phần cứng đã được kích hoạt VÀ chân tín hiệu đang thực sự ở mức CAO */
    if ((s_raw_trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        s_hold_active = 0U;  /* Hủy bỏ bộ đếm thời gian giữ (hold timer) nếu có chuyển động quay trở lại */

        /* Nếu bộ lọc chưa chạy, bắt đầu đếm thời gian */
        if (s_filter_active == 0U) {
            s_filter_start  = SysTimer_GetTick();
            s_filter_active = 1U;
        }

        /* Nếu tín hiệu duy trì mức CAO đủ lâu (vượt qua ngưỡng RADAR_FILTER_MS) -> Xác nhận CÓ NGƯỜI */
        if ((SysTimer_GetTick() - s_filter_start) >= RADAR_FILTER_MS) {
            s_validated_state = RADAR_PRESENCE;
        }
    }
    /* TRƯỜNG HỢP 2: Chân tín hiệu ở mức THẤP hoặc chưa từng có cờ ngắt nào xảy ra */
    else
    {
        s_filter_active = 0U; /* Xóa bộ đếm lọc nhiễu */
        s_raw_trigger   = RADAR_ABSENCE; /* Reset cờ ngắt thô */

        /*
         * BỘ ĐẾM GIỮ TRẠNG THÁI (HOLD TIMER):
         * Đặc tính của cảm biến RCWL-0516 là tự động kéo chân OUT xuống mức THẤP
         * sau khoảng 2-3 giây kể từ lần phát hiện cuối, ngay cả khi người vẫn đứng im trong vùng quét.
         * Do đó, phần mềm cần duy trì trạng thái CÓ NGƯỜI thêm một khoảng thời gian (RADAR_HOLD_MS)
         * trước khi chính thức kết luận là xe đã trống.
         */
        if (s_validated_state == RADAR_PRESENCE) {
        	/* Bắt đầu kích hoạt bộ đếm thời gian giữ */
            if (s_hold_active == 0U) {
                s_hold_start  = SysTimer_GetTick();
                s_hold_active = 1U;
            }
            /* Nếu đã vượt quá thời gian giữ mà không có chuyển động mới -> Xác nhận KHÔNG CÓ NGƯỜI */
            if ((SysTimer_GetTick() - s_hold_start) >= RADAR_HOLD_MS) {
                s_validated_state = RADAR_ABSENCE;
                s_hold_active     = 0U;
            }
        }
    }
    return s_validated_state; /* Trả về trạng thái đã được xử lý tinh gọn cho State Machine */
}

/* ------------------------------------------------------------------ */
/* Public: Hàm Xóa/Khôi Phục Toàn Bộ Trạng Thái Hệ Thống                                       */
/* ------------------------------------------------------------------ */
void Radar_ClearPresence(void)
{
	/* Đặt lại toàn bộ các biến quản lý trạng thái và bộ đếm về mặc định */
    s_raw_trigger     = RADAR_ABSENCE;
    s_validated_state = RADAR_ABSENCE;
    s_filter_active   = 0U;
    s_hold_active     = 0U;
}
