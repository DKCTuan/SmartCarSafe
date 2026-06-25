/**
 * @file    radar_exti.h
 * @author  Mem 2
 * @brief   Driver for RCWL-0516 Radar sensor using EXTI1 (PA1).
 * @date    2026
 */
#ifndef RADAR_EXTI_H
#define RADAR_EXTI_H

#include "stm32f4xx.h"
#include <stdint.h>
#include "sys_timer.h"

/* ------------------------------------------------------------------ */
/* Hardware Pin Definitions - Chỉ chỉnh sửa tại đây khi muốn đổi chân.										  */
/* */
/* !!! WARNING: Nếu thay đổi chân 'RADAR_PIN', phải: 				  										  */
/* 1. Đổi tên hàm phục vụ ngắt (ISR) trong file 'stm32f4xx_it.c' sao cho khớp với bảng Vector ngắt của STM32: */
/* Tên ISR trong stm32f4xx_it.c theo bảng sau:                   */
/*     Pin 0       -> EXTI0_IRQHandler                                */
/*     Pin 1       -> EXTI1_IRQHandler        <- Đang dùng            */
/*     Pin 2       -> EXTI2_IRQHandler                                */
/*     Pin 3       -> EXTI3_IRQHandler                                */
/*     Pin 4       -> EXTI4_IRQHandler                                */
/*     Pin 5-9     -> EXTI9_5_IRQHandler                              */
/*     Pin 10-15   -> EXTI15_10_IRQHandler                       											  */
/* 2. Cập nhật lại macro 'RADAR_EXTI_IRQn' bên dưới tương ứng với nhóm chân.           						  */
/* 3. Cập nhật lại 'RADAR_EXTI_PORT_SRC' nếu chuyển sang sử dụng port khác.    								  */
/* ------------------------------------------------------------------ */
#define RADAR_GPIO_PORT     GPIOA	/**< Cổng GPIO sử dụng cho cảm biến Radar*/
#define RADAR_GPIO_CLK_BIT  0U      /**< Bit bật xung nhịp trong RCC->AHB1ENR: 0=GPIOA, 1=GPIOB, 2=GPIOC*/
#define RADAR_EXTI_PORT_SRC 0U      /**< Mã chọn cổng trong SYSCFG->EXTICR: 0=PA, 1=PB, 2=PC*/
#define RADAR_PIN           1U      /**< Số thứ tự chân GPIO (Ví dụ ở đây là chân số 1 -> PA1)*/
#define RADAR_EXTI_IRQn     EXTI1_IRQn /**< Kênh ngắt tương ứng trong bộ quản lý ngắt NVIC */

/* ------------------------------------------------------------------ */
/* Return Value Definitions                                            */
/* ------------------------------------------------------------------ */
#define RADAR_PRESENCE      1U      /**< Có sự hiện diện: Phát hiện chuyển động hợp lệ (đã lọc nhiễu) */
#define RADAR_ABSENCE       0U      /**< Không có sự hiện diện: Không có chuyển động nào trong vùng quét            */

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Khởi tạo cấu hình phần cứng cho cảm biến Radar.
 * @note   Hàm này sẽ cấu hình chân GPIO ở chế độ Input Pull-down, kích hoạt
 * xung nhịp cho bộ cấu hình hệ thống (SYSCFG), thiết lập ngắt EXTI
 * kích hoạt theo sườn lên (Rising Edge) và cho phép ngắt trên NVIC.
 */
void    Radar_EXTI_Init(void);

/**
 * @brief  Hàm xử lý gọi ngược (Callback) khi xảy ra ngắt ngoài.
 * @warning Bắt buộc phải gọi hàm này bên trong hàm phục vụ ngắt gốc `EXTI1_IRQHandler`
 * ở file `stm32f4xx_it.c` sau khi đã xóa cờ ngắt.
 */
void    Radar_EXTI_Callback(void);

/**
 * @brief  Hàm kiểm tra sự hiện diện có áp dụng bộ lọc thời gian.
 * @retval RADAR_PRESENCE (Có chuyển động) hoặc RADAR_ABSENCE (An toàn).
 * @note   Nên gọi kiểm tra định kỳ (Polling) mỗi 10ms đến 50ms bên trong trạng thái
 * quét dữ liệu (`STATE_SCANNING`) để bộ lọc hoạt động chính xác, tránh báo động giả
 * do nhiễu tức thời của cảm biến vi sóng.
 */
uint8_t Radar_Is_Detected(void);

/**
 * @brief  Xóa sạch các trạng thái lưu trữ nội bộ (Reset bộ lọc nhiễu).
 * @note   Cần gọi hàm này ngay khi hệ thống chuyển về trạng thái nghỉ (`STATE_IDLE`)
 * để đảm bảo bộ đếm thời gian của Radar được làm mới cho lần quét tiếp theo.
 */
void    Radar_ClearPresence(void);

#endif /* RADAR_EXTI_H */
