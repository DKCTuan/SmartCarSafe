/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   RCWL-0516 Radar sensor driver mapped to PA1 (EXTI1) with non-blocking filter.
 * @date    2026
 */

#include "radar_exti.h"

/* ------------------------------------------------------------------ */
/* Timing Configuration Constants                                     */
/* ------------------------------------------------------------------ */
#define RADAR_FILTER_MS     60U    /**< Input signal must stay HIGH >= 60ms to filter out spikes */
#define RADAR_HOLD_MS     3000U    /**< Hold PRESENCE state for 3s after pin goes LOW (sensor hardware constraint) */

/* ------------------------------------------------------------------ */
/* Internal Module Variables                                          */
/* ------------------------------------------------------------------ */
static volatile uint8_t  s_raw_trigger       = RADAR_ABSENCE;
static          uint8_t  s_validated_state   = RADAR_ABSENCE;

/* Time filter control flags and variables */
static          uint8_t  s_filter_active     = 0U; /**< 1 if the 60ms validation filter is running */
static          uint32_t s_filter_start      = 0U; /**< Timestamp when the input signal first went HIGH */

/* Hold timer control flags and variables */
static          uint8_t  s_hold_active       = 0U; /**< 1 if the 3-second state preservation timer is running */
static          uint32_t s_hold_start        = 0U; /**< Timestamp when the signal dropped back to LOW */

/* ------------------------------------------------------------------ */
/* Driver Implementations                                             */
/* ------------------------------------------------------------------ */

void Radar_EXTI_Init(void){
    /* 1. ENABLE PERIPHERAL CLOCKS */
    RCC->AHB1ENR |= (1U << 0);    /* Enable GPIOA clock */
    RCC->APB2ENR |= (1U << 14);   /* Enable SYSCFG clock */

    /* 2. CONFIGURE PA1 AS INPUT WITH PULL-DOWN */
    GPIOA->MODER &= ~(3U << (1 * 2));  /* Set PA1 to Input Mode (00) */
    GPIOA->PUPDR &= ~(3U << (1 * 2));  /* Clear existing PUPDR bits for Pin 1 */
    GPIOA->PUPDR |=  (2U << (1 * 2));  /* Set PA1 to Pull-Down Mode (10) */

    /* 3. CONFIGURE EXTI LINE 1 (For Pin 1) */
    /* Map EXTI1 line to Port A (PA1) by clearing bits [7:4] in EXTICR[0] */
    SYSCFG->EXTICR[0] &= ~(15U << (1 * 4));

    EXTI->IMR  |=  (1U << 1);     /* Unmask EXTI1 interrupt line */
    EXTI->RTSR |=  (1U << 1);     /* Enable Rising Edge trigger for Line 1 */
    EXTI->FTSR &= ~(1U << 1);     /* Disable Falling Edge trigger for Line 1 */
    EXTI->PR    =  (1U << 1);     /* Clear pending flags for Line 1 */

    /* 4. CONFIGURE NVIC FOR EXTI1 */
    NVIC_SetPriority(EXTI1_IRQn, 2);   /* Set EXTI1 priority to 2 */
    NVIC_EnableIRQ(EXTI1_IRQn);        /* Enable EXTI1 global interrupt */
}

void Radar_EXTI_Callback(void){
    s_raw_trigger = RADAR_PRESENCE;    /* Acknowledge raw hardware interrupt trigger */
}

uint8_t Radar_Is_Detected(void){
    /* Read hardware pin state of PA1 from Input Data Register */
    uint8_t pin_high = (GPIOA->IDR & (1U << 1)) ? 1U : 0U;

    /* CASE 1: Raw interrupt triggered AND pin is physically HIGH */
    if ((s_raw_trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        /* New detection resets any ongoing hold timer */
        s_hold_active = 0U;

        /* Start filter timer only once per detection event */
        if (s_filter_active == 0U) {
            s_filter_start  = g_tick_ms;
            s_filter_active = 1U;
        }

        /* Confirm presence only if signal stays HIGH for >= RADAR_FILTER_MS */
        if ((g_tick_ms - s_filter_start) >= RADAR_FILTER_MS) {
            s_validated_state = RADAR_PRESENCE; /* Motion officially validated */
        }
    }
    /* CASE 2: Pin dropped LOW or no raw trigger */
    else
    {
        /* Reset filter - signal was too short, likely noise */
        s_filter_active = 0U;
        s_raw_trigger   = RADAR_ABSENCE;

        /* * HOLD TIMER: RCWL-0516 pulls OUT pin LOW after ~2-3s automatically,
         * even if a person is still present. We hold PRESENCE state for
         * RADAR_HOLD_MS to avoid false ABSENCE during that window.
         */
        if (s_validated_state == RADAR_PRESENCE) {
            if (s_hold_active == 0U) {
                s_hold_start  = g_tick_ms;
                s_hold_active = 1U;
            }

            /* Only clear presence after hold period expires */
            if ((g_tick_ms - s_hold_start) >= RADAR_HOLD_MS) {
                s_validated_state = RADAR_ABSENCE;
                s_hold_active     = 0U;
            }
        }
    }
    return s_validated_state;
}

void Radar_ClearPresence(void){
    /* Hard reset all internal state machine flags and timers */
    s_raw_trigger     = RADAR_ABSENCE;
    s_validated_state = RADAR_ABSENCE;
    s_filter_active   = 0U;
    s_hold_active     = 0U;
}
