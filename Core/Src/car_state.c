#include "car_state.h"

/* Mảng lưu trữ cấu hình chân được nạp từ hàm init */
static Car_Pin_Config_t signal_pins[CAR_SIGNAL_MAX];

/* Biến phục vụ thuật toán debounce
 * Chỉ số mảng tương ứng với giá trị của enum Car_Signal_Type_t */
static uint8_t  last_pin_states[CAR_SIGNAL_MAX]   = {1, 1, 1}; /* Mặc định mức cao (do pull-up) */
static uint8_t  stable_pin_states[CAR_SIGNAL_MAX] = {1, 1, 1}; /* Trạng thái đã lọc nhiễu */
static uint32_t last_debounce_time[CAR_SIGNAL_MAX] = {0, 0, 0}; /* Mốc thời gian SysTick */

/* Hàm cấu hình một chân bất kỳ sang input pull-up */
static void Init_Input_Pullup_Pin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    /* Bật clock tương ứng cho port */
    if      (GPIOx == GPIOA) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; }
    else if (GPIOx == GPIOB) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; }
    else if (GPIOx == GPIOC) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; }

    /* Cấu hình moder: chuyển về 00 (input) */
    GPIOx->MODER &= ~(3U << (pin * 2));

    /* Cấu hình pull-up: xóa cũ, ghi 01 (pull-up) */
    GPIOx->PUPDR &= ~(3U << (pin * 2));
    GPIOx->PUPDR |=  (1U << (pin * 2));
}

/* Hàm cấu hình chân bất kỳ về trạng thái analog mode */
static void DeInit_Input_Pin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    /* Trở về analog mode (11) */
    GPIOx->MODER |=  (3U << (pin * 2));

    /* Tắt pull-up/pull-down (00) */
    GPIOx->PUPDR &= ~(3U << (pin * 2));
}

/* Hàm khởi tạo các chân tín hiệu đầu vào dựa trên mảng cấu hình phần cứng truyền từ main.c
 * Hàm tự động cấp xung nhịp và thiết lập chế độ input pull-up cho các chân tương ứng */
void Car_State_Init(Car_Pin_Config_t *config_array) {
    /* Quét qua toàn bộ mảng cấu hình được truyền vào từ main.c */
    for (uint8_t i = 0; i < CAR_SIGNAL_MAX; i++) {
        /* Lưu lại cấu hình vào bộ nhớ nội bộ */
        signal_pins[i].port = config_array[i].port;
        signal_pins[i].pin  = config_array[i].pin;

        /* Gọi hàm gán chân vật lý */
        Init_Input_Pullup_Pin(signal_pins[i].port, signal_pins[i].pin);
    }
}

/* Hàm hủy cấu hình các chân tín hiệu đầu vào.
 * Cấu hình chân về trạng thái analog mode mặc định và ngắt điện trở kéo */
void Car_State_DeInit(void) {
    /* Quét qua toàn bộ mảng chân và gọi hàm deinit động */
    for (uint8_t i = 0; i < CAR_SIGNAL_MAX; i++) {
        DeInit_Input_Pin(signal_pins[i].port, signal_pins[i].pin);
    }
}

/* Hàm đọc và trả về trạng thái logic an toàn của hệ thống.
 * Có áp dụng thuật toán chống dội phím */
uint8_t Car_Get_System_Status(Car_Signal_Type_t signal_type) {
    uint8_t raw_read = 1;
    uint8_t index = (uint8_t)signal_type;
    uint8_t final_status = 0;

    /* Kiểm tra tính hợp lệ của tham số đầu vào */
    if (signal_type >= CAR_SIGNAL_MAX) {
        return 0;
    }

    /* Trích xuất port và pin từ mảng lưu trữ */
    GPIO_TypeDef *port = signal_pins[index].port;
    uint8_t       pin  = signal_pins[index].pin;
    uint32_t      pin_mask = (1U << pin);

    /* Đọc trạng thái vật lý tức thời từ thanh ghi IDR */
    if (port->IDR & pin_mask) {
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
            /* ACC: nhấn (0) = bật máy (1). nhả (1) = tắt máy (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_DOOR_SIGNAL:
            /* Door: nhấn (0) = cửa mở (1). nhả (1) = cửa đóng (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        case CAR_LOCK_SIGNAL:
            /* Lock: nhấn (0) = đã chốt khóa (1). nhả (1) = chưa khóa (0) */
            final_status = (stable_pin_states[index] == 0) ? 1 : 0;
            break;

        default:
            final_status = 0;
            break;
    }

    return final_status;
}
