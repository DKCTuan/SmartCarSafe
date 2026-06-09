/**
  ******************************************************************************
  * @file           : dev_sound_analog.c
  * @brief          : Sound sensor driver implementation using STM32F411 ADC1
  ******************************************************************************
  */

/* Includes */
#include "dev_sound_analog.h"

/* Private define */
#define MAX_SAMPLES 5U
#define DEBOUNCE_CONFIRM 3U

/* Private variables */

/* Các biến static cục bộ cho thuật toán lọc. */
static uint32_t sg_adcSum = 0;
static uint8_t sg_sampleCount = 0;
static uint8_t sg_noiseConfirmCount = 0;
static uint16_t sg_soundThreshold = 2500U;
static volatile uint16_t sg_soundRawValue = 0;

/* Exported functions */

void Sound_ADC_Init(uint8_t soundPinPos, uint8_t alarmPinPos)
{
    /* 1. GPIO Configuration */
    /* Bật xung clock cho Port A */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Cấu hình chân cảm biến âm thanh*/
    SOUND_PORT->MODER &= ~(3U << soundPinPos);
    SOUND_PORT->MODER |=  (3U << soundPinPos);
    /* Cấu hình chân test còi */
    SOUND_PORT->MODER &= ~(3U << alarmPinPos);
    SOUND_PORT->MODER |=  (1U << alarmPinPos);

    /* 2. ADC1 Configuration (Cấu hình bộ ADC)                                */
    /* Bật xung clock cấp điện cho bộ ngoại vi ADC1 */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* Cấu hình bộ chia xung cho ADC: ADC Clock = PCLK2 / 4 */
    ADC->CCR |= ADC_CCR_ADCPRE_0;

    /* Chọn chế độ Chuyển đổi liên tục (Continuous Conversion Mode) */
    ADC1->CR2 |= ADC_CR2_CONT;

    /* Chọn độ phân giải 12-bit (0 - 4095) */
    ADC1->CR1 &= ~ADC_CR1_RES;

    /* Chọn Kênh 0 (PA0) làm lượt quét đầu tiên */
    ADC1->SQR3 = 0U;

    /* Kích hoạt (Bật nguồn) bộ ADC1 hoạt động */
    ADC1->CR2 |= ADC_CR2_ADON;

    for (volatile uint32_t i = 0; i < 1000; i++)
    {
        __NOP();
    }

    /* Phát lệnh bắt đầu chuyển đổi ADC liên tục */
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void Sound_Process_Sample(void)
{
    if ((ADC1->SR & ADC_SR_EOC) != 0U)
    {
    	sg_adcSum += ADC1->DR;
    	sg_sampleCount++;

    	if (sg_sampleCount >= MAX_SAMPLES)
    	{
    		sg_soundRawValue = (uint16_t)(sg_adcSum / MAX_SAMPLES);

    		sg_adcSum = 0;
    		sg_sampleCount = 0;
    	}
    }
}

uint8_t Sound_Is_Detected(void)
{
	if (sg_soundRawValue > sg_soundThreshold)
	    {
	    	sg_noiseConfirmCount++;

	    	if (sg_noiseConfirmCount >= DEBOUNCE_CONFIRM)
	    	{
	    		return 1U;
	    	}
	    }
	    else
	    {
	    	sg_noiseConfirmCount = 0;
	    }

    return 0U;
}

void Sound_SetThreshold(uint16_t threshold)
{
	sg_soundThreshold = threshold;
}

uint16_t Sound_GetRawValue(void)
{
    return sg_soundRawValue;
}
