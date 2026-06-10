/**
  ******************************************************************************
  * @file           : dev_sound_analog.h
  * @brief          : Header file for sound sensor driver using ADC1
  ******************************************************************************
  */

#ifndef DEV_SOUND_ANALOG_H
#define DEV_SOUND_ANALOG_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Private define ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Cấu trúc lưu cấu hình cho cảm biến âm thanh.
 *
 * Chỉ cần khai báo:
 * - Port GPIO sử dụng
 * - Chân GPIO
 * - ADC tương ứng
 * - ADC Channel
 * */
typedef struct
{
	GPIO_TypeDef *port;
	uint8_t pin;
	ADC_TypeDef *adc;
	uint8_t adcChannel;
}Sound_Config_t;

/**
  * @brief  Processes and reads the latest ADC conversion sample.
  * @retval None
  */
void Sound_Process(void);

/**
  * @brief  Checks if the sound intensity exceeds the threshold.
  * @retval 1 if sound detected, 0 otherwise
  */
uint8_t Sound_IsDetected(void);

/**
 * @brief Sets a dynamic threshold for sound detection.
 * @param threshold: Threshold value (0 - 4095)
 * @retval None
 */
void Sound_SetThreshold(uint16_t threshold);

/**
  * @brief  Gets the current filtered raw value of the sound sensor.
  * @retval 12-bit ADC value (0 - 4095)
  */
uint16_t Sound_GetValue(void);

/**
  * @brief  Initializes ADC1 and configures flexible GPIO pins for sound sensor and alarm.
  * @param  config Con trỏ chứa cấu hình phần cứng
  * @retval None
  */
void Sound_Init(const Sound_Config_t *config);

#endif /* DEV_SOUND_ANALOG_H */
