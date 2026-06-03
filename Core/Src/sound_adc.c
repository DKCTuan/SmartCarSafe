#include "sound_adc.h"


void Sound_ADC_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    SOUND_PORT->MODER |= (3U << SOUND_PIN_POS);
}

void Sound_Process_Sample(void)
{

}

uint8_t Sound_Is_Detected(void)
{
    return 0;
}
