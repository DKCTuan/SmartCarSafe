/**
  ******************************************************************************
  * @file           : adc.h
  * @brief          : Peripheral Driver for STM32F411 ADC Configuration
  ******************************************************************************
  */
#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"

/* Cấu trúc cấu hình phần cứng cơ sở */
typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    uint32_t adcChannel;
    ADC_TypeDef *adc;
} ADC_PeripheralConfig_t;

/* API khởi tạo ADC hệ thống */
void ADC_Peripheral_Init(const ADC_PeripheralConfig_t *p_peripheral);

#endif /* ADC_H */
