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
#define SOUND_PORT              GPIOA
#define SOUND_PIN_POS           GPIO_MODER_MODER0_Pos

/* External variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Processes and reads the latest ADC conversion sample.
  * @retval None
  */
void Sound_Process_Sample(void);

/**
  * @brief  Checks if the sound intensity exceeds the threshold.
  * @retval 1 if sound detected, 0 otherwise
  */
uint8_t Sound_Is_Detected(void);

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
uint16_t Sound_GetRawValue(void);

/**
  * @brief  Initializes ADC1 and configures flexible GPIO pins for sound sensor and alarm.
  * @param  soundPinPos: Bit position of the sound sensor pin in MODER.
  * @param  alarmPinPos: Bit position of the alarm pin in MODER.
  * @retval None
  */
void Sound_ADC_Init(uint8_t soundPinPos, uint8_t alarmPinPos);
#endif /* DEV_SOUND_ANALOG_H */
