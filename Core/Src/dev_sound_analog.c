/**
  ******************************************************************************
  * @file           : dev_sound_analog.c
  * @brief          : Implementation of Sound sensor logic and moving average filter
  ************************************************----------------**************
  */
#include "dev_sound_analog.h"

#define MAX_SAMPLES         5U
#define DEBOUNCE_CONFIRM    3U

/* Biến lưu trữ cấu hình nội bộ */
static Sound_Config_t sg_soundDeviceConfig;

/* Biến phục vụ thuật toán DSP */
static uint32_t sg_adcSum = 0;
static uint8_t sg_sampleCount = 0;
static uint8_t sg_noiseConfirmCount = 0;
static uint16_t sg_soundThreshold = 2500U;
static volatile uint16_t sg_soundRawValue = 0;

/**
  * @brief  Hàm khởi tạo thực thi: Chuyển tiếp yêu cầu xuống tầng phần cứng ADC
  */
void Sound_Init(const Sound_Config_t *config)
{
    if (config != (void *)0)
    {
        /* Sao lưu cấu hình thiết bị */
        sg_soundDeviceConfig = *config;

        /* Gọi hàm khởi tạo của tầng dưới (Peripheral Driver Layer) */
        ADC_Peripheral_Init(&sg_soundDeviceConfig);
    }
}

/**
  * @brief  Thuật toán lọc trung bình cộng lấy mẫu dữ liệu từ tầng phần cứng
  */
void Sound_Process(void)
{
    if ((sg_soundDeviceConfig.adc->SR & ADC_SR_EOC) != 0U)
    {
        sg_adcSum += sg_soundDeviceConfig.adc->DR;
        sg_sampleCount++;

        if (sg_sampleCount >= MAX_SAMPLES)
        {
            sg_soundRawValue = (uint16_t)(sg_adcSum / MAX_SAMPLES);
            sg_adcSum = 0;
            sg_sampleCount = 0;
        }
    }
}

/**
  * @brief  Bộ lọc chống dội bắt trạng thái âm thanh thực tế (Single Return)
  */
uint8_t Sound_IsDetected(void)
{
    uint8_t detectStatus = 0U;

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

/**
  * @brief  Thiết lập và cập nhật ngưỡng so sánh cường độ âm thanh cảnh báo (Cấp Code Convention).
  * @param  threshold: Giá trị số hóa mong muốn thiết lập (0 - 4095).
  * @retval None
  */
void Sound_SetThreshold(uint16_t threshold)
{
    sg_soundThreshold = threshold;
}

/**
  * @brief  Lấy giá trị điện áp số hóa hiện tại sau khi đã qua bộ lọc mịn (Cấp Code Convention).
  * @retval Giá trị biên độ âm thanh 12-bit (0 - 4095).
  */
uint16_t Sound_GetValue(void)
{
    uint16_t value = 0U;
    value = sg_soundRawValue;
    return value;
}
