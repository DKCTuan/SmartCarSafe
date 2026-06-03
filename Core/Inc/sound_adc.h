/*
 * sound_adc.h
 *
 *  Created on: Jun 3, 2026
 *      Author: DINH KIEU CONG TUAN
 */

#ifndef SOUND_ADC_H
#define SOUND_ADC_H

#include "stm32f4xx.h"

/* Tuấn sẽ định nghĩa các hàm API đọc ADC âm thanh ở đây sau */
void Sound_ADC_Init(void);
void Sound_Process_Sample(void);
uint8_t Sound_Is_Detected(void);

#endif /* SOUND_ADC_H */
