/**
  ******************************************************************************
  * @file           : adc.c
  * @brief          : Peripheral Driver implementation for ADC
  ******************************************************************************
  */
#include "adc.h"

/**
  * @brief  Kích hoạt ngoại vi xung nhịp (Clock) cho cổng GPIO bằng toán tử địa chỉ.
  */
static void ADC_GPIO_Clock_Enable(GPIO_TypeDef *port)
{
    if ((port >= GPIOA) && (port <= GPIOH))
    {
        uint32_t portIndex = ((uint32_t)port - (uint32_t)GPIOA) / 0x400UL;
        RCC->AHB1ENR |= (1U << portIndex);
    }
}

/**
  * @brief  Khởi tạo cấu hình cứng thanh ghi cho bộ ADC hoạt động.
  */
void ADC_Peripheral_Init(const ADC_PeripheralConfig_t *p_peripheral)
{
    if (p_peripheral != (void *)0)
    {
        /* 1. Cấu hình phần cứng GPIO */
        ADC_GPIO_Clock_Enable(p_peripheral->port);
        p_peripheral->port->MODER &= ~(3U << (p_peripheral->pin * 2U));
        p_peripheral->port->MODER |=  (3U << (p_peripheral->pin * 2U));

        /* 2. Cấu hình thanh ghi chức năng bộ ADC */
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
        ADC->CCR |= ADC_CCR_ADCPRE_0; /* Chia xung PCLK2 / 4 */

        p_peripheral->adc->CR2 |= ADC_CR2_CONT;  /* Chế độ chạy liên tục */
        p_peripheral->adc->CR1 &= ~ADC_CR1_RES;   /* Độ phân giải 12-bit */
        p_peripheral->adc->SQR3 = p_peripheral->adcChannel; /* Gán kênh quét */

        p_peripheral->adc->CR2 |= ADC_CR2_ADON;  /* Bật nguồn nguồn ADC */

        p_peripheral->adc->CR2 |= ADC_CR2_SWSTART; /* Phát lệnh kích hoạt dịch mẫu */
    }
}
