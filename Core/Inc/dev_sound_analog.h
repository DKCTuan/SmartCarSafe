/**
  ******************************************************************************
  * @file           : dev_sound_analog.h
  * @brief          : Device Driver for Sound Sensor (Logic & DSP Layer)
  ******************************************************************************
  */
#ifndef DEV_SOUND_ANALOG_H
#define DEV_SOUND_ANALOG_H

#include "adc.h" /* Kéo tầng driver ngoại vi vào */

/* Định nghĩa lại kiểu dữ liệu cấu hình để main dễ gọi đồng bộ */
typedef ADC_PeripheralConfig_t Sound_Config_t;

/* Các API xử lý logic nghiệp vụ của cảm biến âm thanh */
void Sound_Init(const Sound_Config_t *config);
void Sound_Process(void);
uint8_t Sound_IsDetected(void);
void Sound_SetThreshold(uint16_t threshold);
uint16_t Sound_GetValue(void);

#endif /* DEV_SOUND_ANALOG_H */
