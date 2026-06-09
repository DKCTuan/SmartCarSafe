#include "car_state.h"

/* Biến toàn cục cục bộ (static) phục vụ thuật toán debounce
 * Chỉ số mảng tương ứng với giá trị của enum Car_Signal_Type_t */
static uint8_t  last_pin_states[CAR_SIGNAL_MAX]   = {1, 1, 1}; /* Mặc định mức cao (do pull-up) */
static uint8_t  stable_pin_states[CAR_SIGNAL_MAX] = {1, 1, 1}; /* Trạng thái đã lọc nhiễu */
static uint32_t last_debounce_time[CAR_SIGNAL_MAX] = {0, 0, 0}; /* Mốc thời gian SysTick */

/* Định nghĩa các hàm chính */
void Car_State_Init(void) {
    /* Bật xung nhịp (clock) cho port C */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* Cấu hình chế độ đầu vào (input mode) cho PC0, PC1, PC2
     * Xóa các bit cấu hình cũ để đưa về mode 00 (input) */
    CAR_STATE_PORT->MODER &= ~(GPIO_MODER_MODER0 |
                               GPIO_MODER_MODER1 |
                               GPIO_MODER_MODER2);

    /* Cấu hình điện trở kéo lên (pull-up) cho PC0, PC1, PC2
     * Xóa trạng thái pull-up/pull-down cũ */
    CAR_STATE_PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR0 |
                               GPIO_PUPDR_PUPDR1 |
                               GPIO_PUPDR_PUPDR2);

    /* Ghi giá trị 01 để kích hoạt pull-up */
    CAR_STATE_PORT->PUPDR |= (GPIO_PUPDR_PUPDR0_0 |
                              GPIO_PUPDR_PUPDR1_0 |
                              GPIO_PUPDR_PUPDR2_0);
}

uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type) {
    uint32_t pin_mask = 0;
    uint8_t  raw_read = 1;
    uint8_t  index = (uint8_t)signal_type;
    uint8_t  final_status = 0;

    /* Kiểm tra tính hợp lệ của tham số đầu vào */
    if (signal_type >= CAR_SIGNAL_MAX) {
        return 0;
    }

    /* Ánh xạ enum sang macro mặt nạ bit của từng chân vật lý */
    switch (signal_type) {
        case CAR_ACC_SIGNAL:
            pin_mask = GPIO_IDR_IDR_0; /* Mặt nạ bit cho PC0 */
            break;
        case CAR_DOOR_SIGNAL:
            pin_mask = GPIO_IDR_IDR_1; /* Mặt nạ bit cho PC1 */
            break;
        case CAR_LOCK_SIGNAL:
            pin_mask = GPIO_IDR_IDR_2; /* Mặt nạ bit cho PC2 */
            break;
        default:
            return 0;
    }

    /* Đọc trạng thái vật lý tức thời từ thanh ghi IDR */
    if (CAR_STATE_PORT->IDR & pin_mask) {
        raw_read = 1; /* Nút chưa nhấn (mức cao do pull-up) */
    } else {
        raw_read = 0; /* Nút đang nhấn (bị kéo xuống GND) */
    }

    /* Thuật toán lọc nhiễu (debounce) non-blocking */
    if (raw_read != last_pin_states[index]) {
        /* Nếu có sự thay đổi điện áp, cập nhật lại mốc thời gian đếm */
        last_debounce_time[index] = HAL_GetTick();
    }

    /* Nếu tín hiệu giữ ổn định qua mức thời gian quy định (50ms) */
    if ((HAL_GetTick() - last_debounce_time[index]) > DEBOUNCE_TIME_MS) {
        stable_pin_states[index] = raw_read; /* Chốt trạng thái an toàn */
    }

    /* Lưu lại trạng thái để so sánh cho chu kỳ quét tiếp theo */
    last_pin_states[index] = raw_read;

    /* Xử lý logic chức năng (mạch active-low)
     * Nút nhấn xuống GND nên mức 0 là trạng thái kích hoạt (active) */
    switch (signal_type) {
        case CAR_ACC_SIGNAL:
            /* ACC: Nhấn (0) = bật máy (1). Nhả (1) = tắt máy (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_DOOR_SIGNAL:
            /* Door: Nhấn (0) = cửa mở (1). Nhả (1) = cửa đóng (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_LOCK_SIGNAL:
            /* Lock: Nhấn (0) = đã chốt khóa (1). Nhả (1) = chưa khóa (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        default:
            final_status = 0;
            break;
    }

    return final_status;
}

void Car_State_DeInit(void) {
    /* Xóa 2 bit MODER của PC0, PC1, PC2 trước */
    CAR_STATE_PORT->MODER &= ~(GPIO_MODER_MODER0 |
                               GPIO_MODER_MODER1 |
                               GPIO_MODER_MODER2);
    /* Ghi 11 để về Analog mode (trạng thái reset mặc định STM32) */
    CAR_STATE_PORT->MODER |=  (GPIO_MODER_MODER0 |
                               GPIO_MODER_MODER1 |
                               GPIO_MODER_MODER2);

    /* Tắt pull-up/pull-down */
    CAR_STATE_PORT->PUPDR &= ~(GPIO_PUPDR_PUPDR0 |
                               GPIO_PUPDR_PUPDR1 |
                               GPIO_PUPDR_PUPDR2);
}
