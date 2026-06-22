/**
 * @file    sys_timer.h
 * @author  Mem 2
 * @brief   Module quản lý bộ định thời hệ thống (System tick timer).
 * Cung cấp độ phân giải thời gian 1ms (mili-giây) chuẩn xác cho toàn bộ các module khác sử dụng.
 * @note    Bắt buộc phải gọi hàm SysTimer_Init() một lần duy nhất trong hàm main()
 * trước khi tiến hành khởi tạo bất kỳ module ngoại vi nào khác.
 */
#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include <stdint.h>

/* Tự động lựa chọn thư viện HAL hoặc thư viện thanh ghi (bare-metal) dựa trên cấu hình của dự án */
#ifdef USE_HAL_DRIVER
    #include "stm32f4xx_hal.h"
#else
    #include "stm32f4xx.h"
#endif

/* Biến toàn cục lưu trữ thời gian hệ thống. Sử dụng từ khóa 'volatile' vì biến này
 * được thay đổi liên tục bên trong trình phục vụ ngắt (ISR). */
extern volatile uint32_t g_tick_ms;

/**
 * @brief  Cấu hình bộ đếm SysTick để tạo ra ngắt định kỳ mỗi 1ms.
 * Lưu ý: Nếu dự án có định nghĩa macro USE_HAL_DRIVER, hàm này sẽ không thực thi lệnh nào (No-op)
 * bởi vì hàm HAL_Init() của thư viện HAL đã tự động đảm nhiệm việc khởi tạo SysTick.
 * @param  cpu_freq_hz  Tần số hoạt động hiện tại của CPU tính bằng Hz. Truyền vào biến SystemCoreClock.
 */
void SysTimer_Init(uint32_t cpu_freq_hz);

/**
 * @brief  Tăng biến đếm thời gian của hệ thống lên 1 đơn vị.
 * Hàm này bắt buộc phải được gọi bên trong trình phục vụ ngắt hệ thống `SysTick_Handler`
 * (thường nằm trong file stm32f4xx_it.c).
 */
void SysTimer_Increment(void);

/**
 * @brief  Đọc và trả về giá trị thời gian hiện tại của hệ thống tính bằng mili-giây (ms).
 * @retval Số tick đếm được kể từ khi vi điều khiển được cấp nguồn hoặc reset.
 * (Lưu ý: Với kiểu dữ liệu uint32_t, biến đếm sẽ bị tràn và quay vòng về 0 sau khoảng 49 ngày hoạt động liên tục).
 */
uint32_t SysTimer_GetTick(void);

#endif /* SYS_TIMER_H */
