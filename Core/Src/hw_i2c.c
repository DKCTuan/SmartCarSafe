/**
 * @file    hw_i2c.c
 * @author  Mem 3 - Nguyễn Công Trường
 * @brief   Thư viện điều khiển I2C1 ở tầng thanh ghi (Hardware Layer),
 * cung cấp các hàm Init, Start, Stop và Read/Write cơ bản.
 */

#include "hw_i2c.h"

void I2C1_Init(void){
	/* CẤP XUNG NHỊP CHO GPIOB VÀ I2C1 */
	RCC->AHB1ENR |= (1U << 1);
	RCC->APB1ENR |= (1U << 21);

	/* CẤU HÌNH CHÂN PB8 (SCL) VÀ PB9 (SDA) */
	/* Chế độ Alternate Function (AF) */
	GPIOB->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
	GPIOB->MODER |=  ((2U << (8 * 2)) | (2U << (9 * 2)));

	/* Chế độ Open-Drain (Bắt buộc cho I2C) */
	GPIOB->OTYPER |= (1U << 8) | (1U << 9);

	/* Bật điện trở kéo lên (Pull-up) */
	GPIOB->PUPDR &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
	GPIOB->PUPDR |=  ((1U << (8 * 2)) | (1U << (9 * 2)));

	/* Chọn chức năng AF4 (I2C1) cho PB8 và PB9 */
	GPIOB->AFR[1] &= ~((15U << (0 * 4)) | (15U << (1 * 4))); /* AFR[1] là thanh ghi AFRH */
	GPIOB->AFR[1] |=  ((4U << (0 * 4)) | (4U << (1 * 4)));

	/* RESET BỘ I2C1 (SOFTWARE RESET) */
	I2C1->CR1 |= (1U << 15);
	I2C1->CR1 &= ~(1U << 15);

	/* CẤU HÌNH THAM SỐ I2C1 (GIẢ SỬ APB1 CLOCK = 16MHZ) */
	/* Thiết lập tần số xung nhịp ngoại vi (16 MHz) */
	I2C1->CR2 = 16;

	/* Cấu hình thanh ghi điều khiển xung nhịp (CCR) cho chế độ Standard Mode (100kHz) */
	/* Công thức: T_high = CCR * T_PCLK1 => CCR = 16MHz / (2 * 100kHz) = 80 */
	I2C1->CCR = 80;

	/* Cấu hình thời gian sườn lên tối đa (TRISE) */
	/* Công thức: TRISE = (1000ns / T_PCLK1) + 1 = (1000ns / 62.5ns) + 1 = 17 */
	I2C1->TRISE = 17;

	/* BẬT I2C1 */
	I2C1->CR1 |= (1U << 0);
}

void I2C1_Start(void) {
	/* KÍCH HOẠT BỘ TẠO ACK VÀ PHÁT TÍN HIỆU START */
	I2C1->CR1 |= (1U << 10);
	I2C1->CR1 |= (1U << 8);

	/* ĐỢI CHO ĐẾN KHI CỜ SB (START BIT) ĐƯỢC BẬT */
	while (!(I2C1->SR1 & (1U << 0)));
}

void I2C1_Stop(void) {
	/* PHÁT TÍN HIỆU STOP */
	I2C1->CR1 |= (1U << 9);
}

void I2C1_WriteAddress(uint8_t address) {
	/* GỬI ĐỊA CHỈ (ĐÃ CHỨA BIT R/W) */
	I2C1->DR = address;

	/* ĐỢI CHO ĐẾN KHI CỜ ADDR (ADDRESS SENT) ĐƯỢC BẬT */
	while (!(I2C1->SR1 & (1U << 1)));

	/* XÓA CỜ ADDR BẰNG CÁCH ĐỌC SR1 RỒI ĐẾN SR2 */
	uint32_t temp = I2C1->SR1;
	temp = I2C1->SR2;
	(void)temp; /* Ép kiểu void để tránh cảnh báo biến không sử dụng */
}

void I2C1_WriteData(uint8_t data) {
	/* ĐỢI CHO ĐẾN KHI THANH GHI DR TRỐNG (TXE BẬT) */
	while (!(I2C1->SR1 & (1U << 7)));

	/* ĐẨY DỮ LIỆU VÀO THANH GHI */
	I2C1->DR = data;

	/* ĐỢI CHO ĐẾN KHI DỮ LIỆU TRUYỀN XONG (BTF BẬT) */
	while (!(I2C1->SR1 & (1U << 2)));
}

uint8_t I2C1_ReadData_ACK(void) {
	/* BẬT PHẢN HỒI ACK BÁO CHO SLAVE MUỐN ĐỌC TIẾP */
	I2C1->CR1 |= (1U << 10);

	/* ĐỢI ĐẾN KHI THANH GHI NHẬN CÓ DỮ LIỆU (RXNE BẬT) */
	while (!(I2C1->SR1 & (1U << 6)));

	/* ĐỌC VÀ TRẢ VỀ DỮ LIỆU */
	return (uint8_t)I2C1->DR;
}

uint8_t I2C1_ReadData_NACK(void) {
	/* TẮT PHẢN HỒI ACK BÁO CHO SLAVE ĐÂY LÀ BYTE CUỐI */
	I2C1->CR1 &= ~(1U << 10);

	/* SINH TÍN HIỆU STOP NGAY SAU KHI NHẬN XONG */
	I2C1->CR1 |= (1U << 9);

	/* ĐỢI ĐẾN KHI THANH GHI NHẬN CÓ DỮ LIỆU (RXNE BẬT) */
	while (!(I2C1->SR1 & (1U << 6)));

	/* ĐỌC VÀ TRẢ VỀ DỮ LIỆU */
	return (uint8_t)I2C1->DR;
}
