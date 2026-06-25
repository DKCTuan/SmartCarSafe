/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   RCWL-0516 driver with time filter and hold timer. Bare-metal STM32F411RE.
 * @date    2026
 */
#include "radar_exti.h"

/* ------------------------------------------------------------------ */
/* Timing Constants                                                    */
/* ------------------------------------------------------------------ */
#define RADAR_FILTER_MS   20U     /**< Signal must stay HIGH >= 60ms to confirm presence */
#define RADAR_HOLD_MS     3000U   /**< Hold PRESENCE for 3s after pin drops LOW          */

/* ------------------------------------------------------------------ */
/* Internal State Variables                                            */
/* ------------------------------------------------------------------ */
static volatile uint8_t  s_raw_trigger     = RADAR_ABSENCE;
static          uint8_t  s_validated_state = RADAR_ABSENCE;
static          uint8_t  s_filter_active   = 0U;
static          uint32_t s_filter_start    = 0U;
static          uint8_t  s_hold_active     = 0U;
static          uint32_t s_hold_start      = 0U;

/* ------------------------------------------------------------------ */
/* Private: GPIO Configuration                                         */
/* ------------------------------------------------------------------ */
static void Radar_GPIO_Init(void)
{
    /* Enable clock for the GPIO port used by radar */
    RCC->AHB1ENR |= (1U << RADAR_GPIO_CLK_BIT);

    /* Set pin as Input mode (MODER = 00) */
    RADAR_GPIO_PORT->MODER &= ~(3U << (RADAR_PIN * 2));

    /* Set pin as Pull-Down (PUPDR = 10) */
    RADAR_GPIO_PORT->PUPDR &= ~(3U << (RADAR_PIN * 2));
    RADAR_GPIO_PORT->PUPDR |=  (2U << (RADAR_PIN * 2));
}

/* ------------------------------------------------------------------ */
/* Public: EXTI + NVIC Initialization                                  */
/* ------------------------------------------------------------------ */
void Radar_EXTI_Init(void)
{
	/* Step 1: Configure GPIO pin */
	Radar_GPIO_Init();

	/* Step 2: Enable SYSCFG clock for EXTI line mapping */
	RCC->APB2ENR |= (1U << 14);

	/* Step 3: Map EXTI line to radar GPIO port */
	/* Clear 4 bits mapping for this pin first */
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
