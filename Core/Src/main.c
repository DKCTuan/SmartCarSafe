/**
 * @file    main.c
 * @author  Mem 2
 * @brief   Test firmware: RCWL-0516 Radar → LED PA5 (Nucleo onboard)
 * @date    2026
 *
 * PIN CHART:
 *   Radar OUT → PA1  (EXTI1, pull-down)
 *   LED test  → PA5  (LED xanh onboard Nucleo)
 *
 * CÁCH ĐỌC KẾT QUẢ:
 *   LED SÁNG  = Radar phát hiện chuyển động (đã qua lọc 60ms)
 *   LED TẮT   = Không có chuyển động (sau hold 3s)
 *
 * CHỈNH ĐỘ NHẠY / THỜI GIAN:
 *   Vào radar_exti.c, sửa:
 *   - RADAR_FILTER_MS  : thời gian lọc nhiễu (mặc định 60ms)
 *   - RADAR_HOLD_MS    : thời gian giữ sau mất tín hiệu (mặc định 3000ms)
 */

#include "main.h"        /* HAL + CubeMX definitions */
#include "radar_exti.h"
#include "sys_timer.h"

/* ------------------------------------------------------------------ */
/* PIN LED TEST                                                         */
/* ------------------------------------------------------------------ */
#define LED_PORT         GPIOA
#define LED_PIN          5U    /* PA5 – LED xanh onboard Nucleo */

/* ------------------------------------------------------------------ */
/* Hàm nội bộ                                                          */
/* ------------------------------------------------------------------ */
static void LED_Init(void)
{
    RCC->AHB1ENR   |=  RCC_AHB1ENR_GPIOAEN;
    LED_PORT->MODER   &= ~(3U << (LED_PIN * 2U));
    LED_PORT->MODER   |=  (1U << (LED_PIN * 2U)); /* Output */
    LED_PORT->OTYPER  &= ~(1U << LED_PIN);         /* Push-pull */
    LED_PORT->OSPEEDR |=  (2U << (LED_PIN * 2U));  /* High speed */
    LED_PORT->PUPDR   &= ~(3U << (LED_PIN * 2U));  /* No pull */
    LED_PORT->ODR     &= ~(1U << LED_PIN);          /* Tắt ban đầu */
}

static void LED_Set(uint8_t on)
{
    if (on)
        LED_PORT->BSRR = (1U << LED_PIN);           /* Set */
    else
        LED_PORT->BSRR = (1U << (LED_PIN + 16U));   /* Reset */
}

/* Delay non-blocking dùng SysTimer */
static void Delay_ms(uint32_t ms)
{
    uint32_t start = SysTimer_GetTick();
    while ((SysTimer_GetTick() - start) < ms);
}

/* ================================================================== */
/* MAIN                                                                */
/* ================================================================== */
int main(void)
{
    /* 1. HAL bắt buộc – khởi tạo Flash, SysTick HAL */
    HAL_Init();

    /* 2. Cấu hình PLL → 84MHz (do CubeMX generate) */
    SystemClock_Config();

    /* 3. SysTimer – HAL project: hàm này chỉ nhận tham số, không config lại SysTick */
    SysTimer_Init(SystemCoreClock);

    /* 4. Khởi tạo LED và Radar */
    LED_Init();
    Radar_EXTI_Init();

    /* ----------------------------------------------------------------
     * Vòng lặp kiểm tra:
     * - Poll Radar_Is_Detected() mỗi 20ms (đủ nhanh cho bộ lọc 60ms)
     * - LED sáng khi có chuyển động hợp lệ
     * ---------------------------------------------------------------- */
    while (1)
    {
        if (Radar_Is_Detected() == RADAR_PRESENCE)
        {
            LED_Set(1);
        }
        else
        {
            LED_Set(0);
        }

        Delay_ms(20); /* Poll mỗi 20ms */
    }
}

/* ================================================================== */
/* SystemClock_Config – giữ nguyên từ CubeMX, KHÔNG sửa              */
/* ================================================================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 16;
    RCC_OscInitStruct.PLL.PLLN            = 336;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1);
}
