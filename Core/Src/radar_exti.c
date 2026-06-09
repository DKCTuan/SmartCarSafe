/**
 * @file    radar_exti.c
 * @author  Mem 2
 * @brief   RCWL-0516 Radar sensor driver with non-blocking time filter and debounce.
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

    /* 2. CONFIGURE PA0 AS INPUT WITH PULL-DOWN */
    GPIOA->MODER &= ~(3U << (0 * 2));  /* Set PA0 to Input Mode (00) */
    GPIOA->PUPDR &= ~(3U << (0 * 2));  /* Clear existing PUPDR bits first to prevent Reserved state */
    GPIOA->PUPDR |=  (2U << (0 * 2));  /* Set PA0 to Pull-Down Mode (10) */

    /* 3. CONFIGURE EXTI LINE 0 */
    SYSCFG->EXTICR[0] &= ~(15U << (0 * 4)); /* Map EXTI0 line to Port A (PA0) */
    EXTI->IMR  |=  (1U << 0);     /* Unmask EXTI0 interrupt line */
    EXTI->RTSR |=  (1U << 0);     /* Enable Rising Edge trigger (detect motion initiation) */
    EXTI->FTSR &= ~(1U << 0);     /* Disable Falling Edge trigger */
    EXTI->PR    =  (1U << 0);     /* Clear any pending stale interrupt flags */

    /* 4. CONFIGURE NVIC (Nested Vectored Interrupt Controller) */
    NVIC_SetPriority(EXTI0_IRQn, 2);   /* Set interrupt priority to 2 */
    NVIC_EnableIRQ(EXTI0_IRQn);        /* Enable EXTI0 global interrupt */
}

void Radar_EXTI_Callback(void){
    s_raw_trigger = RADAR_PRESENCE;    /* Acknowledge raw hardware interrupt trigger */
}

uint8_t Radar_Is_Detected(void){
    /* Read current hardware pin state of PA0 directly from Input Data Register */
    uint8_t pin_high = (GPIOA->IDR & (1U << 0)) ? 1U : 0U;

    /* CASE 1: Raw interrupt triggered AND hardware pin is confirmed HIGH */
    if ((s_raw_trigger == RADAR_PRESENCE) && (pin_high == 1U))
    {
        s_hold_active = 0U; /* Intercept and abort HOLD countdown if motion re-occurs */

        if (s_filter_active == 0U) {
            s_filter_start  = g_tick_ms; /* Record starting timestamp of the pulse */
            s_filter_active = 1U;        /* Activate the validation filter flag */
        }

        /* * Check if signal has sustained HIGH for >= 60ms.
         * Standard unsigned 32-bit subtraction safely handles SysTick counter overflow wrap-around.
         */
        if ((g_tick_ms - s_filter_start) >= RADAR_FILTER_MS) {
            s_validated_state = RADAR_PRESENCE; /* Motion officially validated */
        }
    }
    /* CASE 2: Hardware pin dropped to LOW or no active raw trigger exists */
    else
    {
        s_filter_active = 0U; /* Reset the 60ms time filter */
        s_raw_trigger   = RADAR_ABSENCE;

        /* If previous state was valid PRESENCE, initiate the 3-second hold buffer */
        if (s_validated_state == RADAR_PRESENCE) {
            if (s_hold_active == 0U) {
                s_hold_start  = g_tick_ms; /* Record starting timestamp of the hold window */
                s_hold_active = 1U;
            }

            /* If 3 seconds have passed without any new motion, revert state back to ABSENCE */
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
