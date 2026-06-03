#ifndef SOUND_ADC_H
#define SOUND_ADC_H

#include "stm32f4xx.h"

#define SOUND_PORT          GPIOA
#define SOUND_PIN_POS       GPIO_MODER_MODER1_Pos

void Sound_ADC_Init(void);
void Sound_Process_Sample(void);
uint8_t Sound_Is_Detected(void);

#endif /* SOUND_ADC_H */
