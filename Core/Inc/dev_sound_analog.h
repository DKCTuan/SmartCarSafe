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
#define SOUND_PIN_POS           GPIO_MODER_MODER1_Pos 

/* External variables --------------------------------------------------------*/
extern volatile uint16_t g_soundRawValue; /* Tiền tố g_ cho biến toàn cục */

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initializes ADC1 peripheral and GPIOA Pin 1 for sound sensor.
  * @retval None
  */
void Sound_ADC_Init(void);

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

#endif /* DEV_SOUND_ANALOG_H */