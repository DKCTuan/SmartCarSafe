/**
 * @file    hw_i2c.c
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Lớp trừu tượng cho I2C (Hỗ trợ các module I2Cx).
 * Định nghĩa các hàm cấu hình phần cứng và thao tác truyền/nhận.
 */

#include "hw_i2c.h"

/**
 * @brief  Cấu hình một chân GPIO bất kỳ sang chế độ I2C (AF, Open-Drain, Pull-up).
 * @param  GPIOx:  Con trỏ tới Port chứa chân đó (VD: GPIOA, GPIOB).
 * @param  pin:    Số thứ tự chân (0 - 15).
 * @param  af_num: Mã Alternate Function (VD: 4 hoặc 9 cho I2C).
 * @retval None
 */
void HW_GPIO_Init_I2C_Pin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t af_num) {
    /* 1. Kích hoạt Clock cho Port GPIO tương ứng một cách tự động */
    if      (GPIOx == GPIOA) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; }
    else if (GPIOx == GPIOB) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; }
    else if (GPIOx == GPIOC) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; }

    /* 2. Cấu hình Moder: Chuyển sang Alternate Function */
    GPIOx->MODER &= ~(3U << (pin * 2));
    GPIOx->MODER |=  (2U << (pin * 2));

    /* 3. Cấu hình Output Type: Open-Drai */
    GPIOx->OTYPER |= (1U << pin);

    /* 4. Cấu hình Pull-up */
    GPIOx->PUPDR &= ~(3U << (pin * 2));
    GPIOx->PUPDR |=  (1U << (pin * 2));

    /* 5. Cấu hình Alternate Function Register (AFR) */
    /* Thanh ghi AFR được chia làm 2: AFR[0] cho chân 0-7, AFR[1] cho chân 8-15 */
    if (pin < 8) {
        GPIOx->AFR[0] &= ~(15U << (pin * 4));
        GPIOx->AFR[0] |=  (af_num << (pin * 4));
    } else {
        uint8_t pin_high = pin - 8;
        GPIOx->AFR[1] &= ~(15U << (pin_high * 4));
        GPIOx->AFR[1] |=  (af_num << (pin_high * 4));
    }
}


/**
 * @brief  Khởi tạo bộ ngoại vi I2C (Cấu hình Baudrate, TRISE, bật I2C).
 * @param  I2Cx: Con trỏ tới ngoại vi I2C cần dùng (I2C1, I2C2, I2C3).
 * @retval None
 */
void HW_I2C_Init(I2C_TypeDef *I2Cx) {
    /* 1. Kích hoạt Clock cho bộ I2C được truyền vào */
    if      (I2Cx == I2C1) { RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; }
    else if (I2Cx == I2C2) { RCC->APB1ENR |= RCC_APB1ENR_I2C2EN; }
    else if (I2Cx == I2C3) { RCC->APB1ENR |= RCC_APB1ENR_I2C3EN; }

    /* 2. Khởi động lại */
    I2Cx->CR1 |= I2C_CR1_SWRST;
    I2Cx->CR1 &= ~I2C_CR1_SWRST;

    /* 3. Cấu hình thông số (APB1 = 16MHz, tốc độ I2C = 100kHz) */
    I2Cx->CR2   = 16U;
    I2Cx->CCR   = 80U;
    I2Cx->TRISE = 17U;

    /* 4. Cho phép I2Cx hoạt động */
    I2Cx->CR1 |= I2C_CR1_PE;
}

/**
 * @brief  Phát điều kiện START trên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval None
 */
void HW_I2C_Start(I2C_TypeDef *I2Cx) {
    /* Kích hoạt cờ tự động trả lời ACK và phát START */
    I2Cx->CR1 |= (I2C_CR1_ACK | I2C_CR1_START);

    /* Chờ đến khi cờ START (SB) được set */
    while ((I2Cx->SR1 & I2C_SR1_SB) == 0U);
}

/**
 * @brief  Phát điều kiện STOP trên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval None
 */
void HW_I2C_Stop(I2C_TypeDef *I2Cx) {
    I2Cx->CR1 |= I2C_CR1_STOP;
}

/**
 * @brief  Gửi địa chỉ của thiết bị Slave.
 * @param  I2Cx:    Con trỏ tới ngoại vi I2C đang sử dụng.
 * @param  address: Địa chỉ 8-bit của thiết bị Slave.
 * @retval None
 */
void HW_I2C_WriteAddress(I2C_TypeDef *I2Cx, uint8_t address) {
    /* Ghi địa chỉ vào thanh ghi dữ liệu */
    I2Cx->DR = address;

    /* Chờ cờ Address (ADDR) bật lên */
    while ((I2Cx->SR1 & I2C_SR1_ADDR) == 0U);

    /* Xóa cờ ADDR bằng chuỗi đọc SR1 -> SR2 */
    volatile uint32_t clear_flag = I2Cx->SR1;
    clear_flag = I2Cx->SR2;
    (void)clear_flag;
}

/**
 * @brief  Gửi 1 byte dữ liệu lên bus I2C.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @param  data: Dữ liệu (8-bit) cần truyền đi.
 * @retval None
 */
void HW_I2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data) {
    /* Chờ thanh ghi dữ liệu trống (TXE) */
    while ((I2Cx->SR1 & I2C_SR1_TXE) == 0U);

    /* Đẩy dữ liệu vào thanh ghi */
    I2Cx->DR = data;

    /* Chờ quá trình truyền hoàn tất (BTF) */
    while ((I2Cx->SR1 & I2C_SR1_BTF) == 0U);
}

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu ACK.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval uint8_t: Dữ liệu đọc được.
 */
uint8_t HW_I2C_ReadData_ACK(I2C_TypeDef *I2Cx) {
    /* Bật phản hồi ACK */
    I2Cx->CR1 |= I2C_CR1_ACK;

    /* Chờ nhận đủ dữ liệu (RXNE) */
    while ((I2Cx->SR1 & I2C_SR1_RXNE) == 0U);

    return (uint8_t)(I2Cx->DR);
}

/**
 * @brief  Đọc 1 byte dữ liệu từ bus I2C và trả về tín hiệu NACK kèm STOP.
 * @param  I2Cx: Con trỏ tới ngoại vi I2C đang sử dụng.
 * @retval uint8_t: Dữ liệu đọc được.
 */
uint8_t HW_I2C_ReadData_NACK(I2C_TypeDef *I2Cx) {
    /* Tắt phản hồi ACK và sinh tín hiệu STOP */
    I2Cx->CR1 &= ~I2C_CR1_ACK;
    I2Cx->CR1 |= I2C_CR1_STOP;

    /* Chờ nhận đủ dữ liệu (RXNE) */
    while ((I2Cx->SR1 & I2C_SR1_RXNE) == 0U);

    return (uint8_t)(I2Cx->DR);
}
