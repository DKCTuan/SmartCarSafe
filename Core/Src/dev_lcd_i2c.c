/*
 * dev_lcd_i2c.c
 *
 * Created on: Jun 13, 2026
 * Author: Mem 3 - Nguyễn Công Trường
 */

#include "dev_lcd_i2c.h"
#include "hw_i2c.h"
#include <stdio.h>

/* ĐỊA CHỈ GIAO TIẾP I2C */
#define LCD_I2C_W_ADDR 		(LCD_I2C_ADDR << 1)

/* ĐỊNH NGHĨA CHÂN CỦA PCF8574 NỐI VỚI LCD */
#define LCD_EN 0x04  /* Chân Enable (P2) */
#define LCD_RW 0x02  /* Chân Read/Write (P1) */
#define LCD_RS 0x01  /* Chân Register Select (P0) */
#define LCD_BL 0x08  /* Chân Backlight (P3) */

/* ĐỊNH NGHĨA CÁC LỆNH ĐIỀU KHIỂN LCD*/
/* Lệnh cơ bản */
#define LCD_CMD_CLEAR_DISPLAY   0x01    /* Xóa sạch toàn bộ màn hình */
#define LCD_CMD_RETURN_HOME     0x02    /* Đưa con trỏ về gốc (0,0) không xóa chữ */

/* Lệnh thiết lập chế độ nhập */
#define LCD_CMD_ENTRY_MODE_INC  0x06    /* Tự động dịch con trỏ sang phải khi in */

/* Lệnh điều khiển hiển thị */
#define LCD_CMD_DISPLAY_ON      0x0C    /* Bật màn hình, tắt con trỏ, không nháy */
#define LCD_CMD_DISPLAY_OFF     0x08    /* Tắt toàn bộ màn hình */

/* Lệnh thiết lập chức năng */
#define LCD_CMD_INIT_8BIT       0x30    /* Lệnh reset phần cứng về 8-bit */
#define LCD_CMD_INIT_4BIT       0x20    /* Lệnh cấu hình phần cứng sang 4-bit */
#define LCD_CMD_4BIT_2LINE      0x28    /* Chế độ 4-bit, 2 dòng hiển thị, font 5x8 */

/* Định nghĩa địa chỉ bộ nhớ */
#define LCD_DDRAM_ROW_1         0x80    /* Địa chỉ gốc của Dòng 1 */
#define LCD_DDRAM_ROW_2         0xC0    /* Địa chỉ gốc của Dòng 2 */

/* CÁC HÀM HỖ TRỢ NỘI BỘ */
static void Delay_ms(uint32_t ms) {
	/* HÀM DELAY NỘI BỘ DỰA TRÊN XUNG HSI 16MHZ */
	for (uint32_t i = 0; i < ms * 1600; i++) {
		__asm("nop");
	}
}

static void LCD_Write_I2C(I2C_TypeDef *I2Cx, uint8_t data) {	/* GỬI 1 BYTE XUỐNG MODULE PCF8574 */
	HW_I2C_Start(I2Cx);

	/* Bắt buộc dịch trái 1 bit để chừa chỗ cho bit Write (0) của chuẩn I2C */
	HW_I2C_WriteAddress(I2Cx, LCD_I2C_W_ADDR);
	HW_I2C_WriteData(I2Cx, data);

	HW_I2C_Stop(I2Cx);
}

static void LCD_Send(I2C_TypeDef *I2Cx, uint8_t data, uint8_t mode) {	/* CHIA DỮ LIỆU 8-BIT THÀNH 2 LẦN GỬI 4-BIT (HIGH NIBBLE & LOW NIBBLE) */
	/* mode = 0 (Ghi lệnh) hoặc mode = LCD_RS (Ghi ký tự) */
	uint8_t high_nibble = (data & 0xF0) | mode | LCD_BL;
	uint8_t low_nibble  = ((data << 4) & 0xF0) | mode | LCD_BL;

	/* GỬI 4-BIT CAO KÈM THEO XUNG CHÂN EN */
	LCD_Write_I2C(I2Cx, high_nibble | LCD_EN);
	Delay_ms(1);
	LCD_Write_I2C(I2Cx, high_nibble & ~LCD_EN);
	Delay_ms(1);

	/* GỬI 4-BIT THẤP KÈM THEO XUNG CHÂN EN */
	LCD_Write_I2C(I2Cx, low_nibble | LCD_EN);
	Delay_ms(1);
	LCD_Write_I2C(I2Cx, low_nibble & ~LCD_EN);
	Delay_ms(1);
}

static void LCD_Send_Command(I2C_TypeDef *I2Cx, uint8_t cmd) {	/* GỬI MỘT MÃ LỆNH ĐIỀU KHIỂN (RS = 0) */
	LCD_Send(I2Cx, cmd, 0);
}

static void LCD_Send_Data(I2C_TypeDef *I2Cx, uint8_t data) {	/* GỬI MỘT KÝ TỰ ĐỂ HIỂN THỊ (RS = 1) */
	LCD_Send(I2Cx, data, LCD_RS);
}

static void LCD_SetCursor(I2C_TypeDef *I2Cx, uint8_t row, uint8_t col) {	/* ĐỊNH VỊ CON TRỎ THEO TỌA ĐỘ HÀNG VÀ CỘT */
	uint8_t address;
	if (row == 0) {
		address = LCD_DDRAM_ROW_1 + col; /* Dòng 1 */
	} else {
		address = LCD_DDRAM_ROW_2 + col; /* Dòng 2 */
	}
	LCD_Send_Command(I2Cx, address);
}

static void LCD_PrintString(I2C_TypeDef *I2Cx, char *str) {	/* IN TỪNG KÝ TỰ CỦA CHUỖI ĐẾN KHI KẾT THÚC (\0) */
	while (*str) {
		LCD_Send_Data(I2Cx, (uint8_t)(*str));
		str++;
	}
}

/* CÁC HÀM GIAO TIẾP VỚI MAIN */
void LCD_Init(I2C_TypeDef *I2Cx) {
	/* BƯỚC KHỞI TẠO THEO DATASHEET CỦA CHIP HD44780 */
	Delay_ms(50);

	/* ÉP MÀN HÌNH VỀ CHẾ ĐỘ 4-BIT */
	LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT | LCD_EN); Delay_ms(5); LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT & ~LCD_EN); Delay_ms(5);
	LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT | LCD_EN); Delay_ms(1); LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT & ~LCD_EN); Delay_ms(5);
	LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT | LCD_EN); Delay_ms(1); LCD_Write_I2C(I2Cx, LCD_CMD_INIT_8BIT & ~LCD_EN); Delay_ms(5);
	LCD_Write_I2C(I2Cx, LCD_CMD_INIT_4BIT | LCD_EN); Delay_ms(1); LCD_Write_I2C(I2Cx, LCD_CMD_INIT_4BIT & ~LCD_EN); Delay_ms(5);

	/* THIẾT LẬP THÔNG SỐ HOẠT ĐỘNG */
	LCD_Send_Command(I2Cx, LCD_CMD_4BIT_2LINE);     /* Chế độ 4-bit, 2 dòng, font 5x8 */
	Delay_ms(1);
	LCD_Send_Command(I2Cx, LCD_CMD_DISPLAY_ON);     /* Bật màn hình, tắt con trỏ, không nháy */
	Delay_ms(1);
	LCD_Send_Command(I2Cx, LCD_CMD_CLEAR_DISPLAY);  /* Xóa sạch màn hình */
	Delay_ms(2);
	LCD_Send_Command(I2Cx, LCD_CMD_ENTRY_MODE_INC); /* Tự động dịch con trỏ sang phải khi viết */
	Delay_ms(1);
}

void LCD_Display_Status(I2C_TypeDef *I2Cx, uint8_t state, float temperature) {
	char temp_str[24];

	/* 1. XÓA MÀN HÌNH ĐỂ TRÁNH RÁC DỮ LIỆU CŨ */
	LCD_Send_Command(I2Cx, LCD_CMD_CLEAR_DISPLAY);
	Delay_ms(2);

	/* 2. HIỂN THỊ TRẠNG THÁI HOẠT ĐỘNG Ở DÒNG 1 */
	LCD_SetCursor(I2Cx, 0, 0);
	switch (state) {
		case SYS_STATE_SAFE:
			LCD_PrintString(I2Cx, "STATUS: SAFE");
			break;
		case SYS_STATE_WARNING:
			LCD_PrintString(I2Cx, "STATUS: WARNING!");
			break;
		case SYS_STATE_DANGER:
			LCD_PrintString(I2Cx, "STATUS: DANGER!!");
			break;
		case SYS_STATE_ERROR:
			LCD_PrintString(I2Cx, "STATUS: ERROR");
			break;
		default:
			LCD_PrintString(I2Cx, "STATUS: UNKNOWN");
			break;
	}

	/* 3. HIỂN THỊ NHIỆT ĐỘ THỰC TẾ Ở DÒNG 2 */
	LCD_SetCursor(I2Cx, 1, 0);

	/* XỬ LÝ KHÔNG DÙNG FLOAT: Tách phần nguyên và phần thập phân */
	int temp_int = (int)temperature;                         // Lấy phần nguyên (VD: 28.5 -> 28)
	int temp_frac = (int)((temperature - temp_int) * 10.0f); // Lấy 1 chữ số thập phân (VD: 0.5 * 10 = 5)

	/* Xử lý trường hợp nhiệt độ âm (VD: -12.5 độ) để phần thập phân không bị âm */
	if (temp_frac < 0) {
		temp_frac = -temp_frac;
	}

	/* In ra bằng %d (số nguyên) thay vì %f */
	snprintf(temp_str, sizeof(temp_str), "Temp: %d.%d C", temp_int, temp_frac);

	LCD_PrintString(I2Cx, temp_str);
}
