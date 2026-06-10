/**
  ******************************************************************************
  * @file           : dev_sound_analog.c
  * @brief          : Sound sensor driver implementation using STM32F411 ADC1
  ******************************************************************************
  */

/* Includes */
#include "dev_sound_analog.h"

/* Private define */
#define MAX_SAMPLES      5U
#define DEBOUNCE_CONFIRM 3U

/* Private variables */

/* Lưu cấu hình cảm biến sau khi Init*/
static Sound_Config_t sg_soundConfig;

/* Các biến static cục bộ cho thuật toán lọc. */
static uint32_t sg_adcSum = 0;
static uint8_t sg_sampleCount = 0;
static uint8_t sg_noiseConfirmCount = 0;
static uint16_t sg_soundThreshold = 2500U;
static volatile uint16_t sg_soundRawValue = 0;

/* Private function */

static void Sound_GPIO_Clock_Enable(GPIO_TypeDef *port)
{
	if(port ==GPIOA)
	{
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	}
	else if(port == GPIOB)
	{
	    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	}
	else if(port == GPIOC)
	{
	    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	}
}

/* Exported functions */

void Sound_Init(const Sound_Config_t * config)
{
	if(config != (void*)0)
	{

		/* Lưu cấu hình sử dụng */
		sg_soundConfig = *config;

		/* 1. GPIO Configuration*/
		/* Bật clock cho GPIO */
		Sound_GPIO_Clock_Enable(config->port);

		/* Cấu hình chân cảm biến sang Analog Mode (11) */
		config->port->MODER &= ~(3U << (config->pin * 2U));
		config->port->MODER |=  (3U << (config->pin * 2U));

		/* 2. ADC Configuration */
		/* Bật clock ADC */
		RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

		/* ADC Clock = PCLK2 / 4 */
		ADC->CCR |= ADC_CCR_ADCPRE_0;

		/* Continuous Conversion Mode */
		config->adc->CR2 |= ADC_CR2_CONT;

		/* Độ phân giải 12-bit */
		config->adc->CR1 &= ~ADC_CR1_RES;

		/* Chọn ADC Channel */
		config->adc->SQR3 = config->adcChannel;

		/* Bật ADC */
		config->adc->CR2 |= ADC_CR2_ADON;

		/* Start ADC conversion */
		config->adc->CR2 |= ADC_CR2_SWSTART;

	}
}

void Sound_Process(void)
{
    if ((sg_soundConfig.adc->SR & ADC_SR_EOC) != 0U)
    {
    	/* Đọc dữ liệu ADC */
    	sg_adcSum += sg_soundConfig.adc->DR;
    	sg_sampleCount++;

    	/* Đủ số mẫu thì tính trung bình */
    	if (sg_sampleCount >= MAX_SAMPLES)
    	{
    		sg_soundRawValue = (uint16_t)(sg_adcSum / MAX_SAMPLES);

    		sg_adcSum = 0;
    		sg_sampleCount = 0;
    	}
    }
}

uint8_t Sound_IsDetected(void)
{
	uint8_t detectStatus  = 0U;

	if (sg_soundRawValue > sg_soundThreshold)
	    {
	    	sg_noiseConfirmCount++;

	    	if (sg_noiseConfirmCount >= DEBOUNCE_CONFIRM)
	    	{
	    		detectStatus = 1U;
	    	}
	    }
	    else
	    {
	    	sg_noiseConfirmCount = 0;
	    }

    return detectStatus;
}

void Sound_SetThreshold(uint16_t threshold)
{
	sg_soundThreshold = threshold;
}

uint16_t Sound_GetValue(void)
{
	uint16_t value = 0U;
	value = sg_soundRawValue;
    return value;
}
