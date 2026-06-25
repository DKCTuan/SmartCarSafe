/**
 * @file    sys_timer.c
 * @author  Mem 2
 * @brief   Triển khai module bộ định thời hệ thống (System tick timer).
 * Tương thích tốt với cả dự án dùng thư viện HAL hoặc lập trình thanh ghi (Bare-metal).
 * @note    Tự động lựa chọn chiến lược khởi tạo dựa vào việc có định nghĩa macro USE_HAL_DRIVER hay không.
 */
#include "sys_timer.h"
/* Biến toàn cục lưu trữ thời gian hệ thống tính bằng mili-giây kể từ lúc khởi động.
 * Sử dụng từ khóa 'volatile' để báo cho trình biên dịch biết biến này có thể bị thay đổi
 * bất ngờ bên trong trình phục vụ ngắt (ISR), tránh việc tối ưu hóa sai dữ liệu. */
volatile uint32_t g_tick_ms = 0;
/**
 * @brief  Khởi tạo cấu hình bộ đếm SysTick để tạo ngắt định kỳ mỗi 1ms.
 * @param  cpu_freq_hz  Tần số hoạt động của CPU tính bằng Hz (Ví dụ: 16000000U cho 16MHz).
 */
void SysTimer_Init(uint32_t cpu_freq_hz)
{
#ifdef USE_HAL_DRIVER
	/*
	 * Dự án sử dụng thư viện HAL: Bộ đếm SysTick đã được cấu hình sẵn bên trong
	 * hàm HAL_Init() tại file main.c. Do đó, bỏ qua việc cấu hình lại ở đây để
	 * tránh ghi đè và làm sai lệch các thiết lập thời gian của thư viện HAL.
	 */
    (void)cpu_freq_hz; /* Ép kiểu void để tránh cảnh báo (warning) trình biên dịch về việc biến không được sử dụng */
#else
    /*
         * Bare-metal: Không có thư viện HAL, cấu hình SysTick thủ công.
         * Hàm SysTick_Config() nhận vào số chu kỳ xung nhịp của CPU trong 1 mili-giây.
         * Công thức: cpu_freq_hz / 1000U (Ví dụ: Nếu CPU chạy 16MHz = 16,000,000 Hz,
         * chia cho 1000 sẽ ra 16,000 chu kỳ xung nhịp cho mỗi 1ms ngắt một lần).
         */
    SysTick_Config(cpu_freq_hz / 1000U); //16MHz
#endif
}
/**
 * @brief  Tăng biến đếm thời gian hệ thống lên 1 đơn vị (tương ứng 1ms).
 * @note   Hàm này bắt buộc phải được gọi bên trong trình phục vụ ngắt hệ thống
 * SysTick_Handler() tại file stm32f4xx_it.c để đảm bảo thời gian tăng tuyến tính.
 */
void SysTimer_Increment(void)
{
    g_tick_ms++;
}
/**
 * @brief  Đọc và trả về giá trị thời gian hiện tại của hệ thống.
 * @retval Số mili-giây (uint32_t) đã trôi qua kể từ khi vi điều khiển được cấp nguồn hoặc reset.
 */
uint32_t SysTimer_GetTick(void)
{
    return g_tick_ms;
}
