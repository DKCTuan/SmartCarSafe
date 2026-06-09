#include "car_actuator.h"

void Car_Actuator_Init(void) {
    /* Bat xung nhip clock cho cac port A, B, C va bo dinh thoi TIM3 */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Cau hinh chan va huong ra cho mach dieu khien dong co TB6612
     * Xoa cac bit cau hinh cu cua chan PA7 (AIN1) va chuyen thanh GPIO Output (01) */
    MOTOR_PORT_A->MODER &= ~GPIO_MODER_MODER7;
    MOTOR_PORT_A->MODER |= GPIO_MODER_MODER7_0;

    /* Xoa cac bit cau hinh cu cua chan PB0 (AIN2), PB1 (STBY) va chuyen thanh GPIO Output (01) */
    MOTOR_PORT_B->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1);
    MOTOR_PORT_B->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0);

    /* Dat cac chan dieu khien chieu ve muc thap ban dau de quat dung */
    MOTOR_PORT_A->ODR &= ~GPIO_ODR_ODR_7;
    MOTOR_PORT_B->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1);

    /* Cau hinh chan chuc nang thay the Alternate Function cho chan PWMA (PA6) va servo (PC8)
     * Chuyen chan PA6 (PWMA) sang che do Alternate Function (10) */
    MOTOR_PORT_A->MODER &= ~GPIO_MODER_MODER6;
    MOTOR_PORT_A->MODER |= GPIO_MODER_MODER6_1;

    /* Chuyen chan PC8 (Servo) sang che do Alternate Function (10) */
    SERVO_PORT->MODER &= ~GPIO_MODER_MODER8;
    SERVO_PORT->MODER |= GPIO_MODER_MODER8_1;

    /* Gan chuc nang AF2 (TIM3) cho chan PA6 thong qua thanh ghi AFRL (AFR[0]) */
    MOTOR_PORT_A->AFR[0] &= ~GPIO_AFRL_AFSEL6;
    MOTOR_PORT_A->AFR[0] |= (2 << GPIO_AFRL_AFSEL6_Pos);

    /* Gan chuc nang AF2 (TIM3) cho chan PC8 thong qua thanh ghi AFRH (AFR[1]) */
    SERVO_PORT->AFR[1] &= ~GPIO_AFRH_AFSEL8;
    SERVO_PORT->AFR[1] |= (2 << GPIO_AFRH_AFSEL8_Pos);

    /* Cau hinh bo dinh thoi TIM3 de tao ra tan so bam xung xung quanh moc 50Hz (chu ky 20ms)
     * Prescaler = 16 - 1 giup clock cua timer giam xuong con 1MHz (moi tick ung voi 1 micro giay) */
    TIM3->PSC = 16 - 1;

    /* Auto-Reload Register nap gia tri 20000 giup chu ky dat dung 20ms (50Hz) */
    TIM3->ARR = 20000 - 1;

    /* Cau hinh kenh 1 (PWMA cua quat) o che do PWM Mode 1 (110) va bat tinh nang dinh nap truoc Preload */
    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

    /* Cau hinh kenh 3 (PWM cua servo) o che do PWM Mode 1 (110) va bat tinh nang dinh nap truoc Preload */
    TIM3->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM3->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

    /* Kich hoat dau ra cho kenh 1 va kenh 3 cua bo dinh thoi TIM3 */
    TIM3->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC3E);

    /* Bat bo dem cua TIM3 de bat dau phat xung */
    TIM3->CR1 |= TIM_CR1_CEN;

    /* Kich hoat Update Event de nap ngay cac gia tri PSC, ARR, CCR vao shadow register */
    TIM3->EGR |= TIM_EGR_UG;

    /* Thiet lap trang thai khoi dau an toan: Quat tat, kinh o vi tri dong hoan toan (0 do) */
    TIM3->CCR1 = 0;
    TIM3->CCR3 = 1000; /* Xung rong 1ms tuong ung voi 0 do cua servo */
}

void Actuator_Set_Fan_Speed(uint8_t speed_level) {
    switch (speed_level) {
        case 0:
            /* Tat quat: Ha chan STBY xuong muc thap de ngat mach cau H */
            MOTOR_PORT_B->ODR &= ~GPIO_ODR_ODR_1;
            MOTOR_PORT_A->ODR &= ~GPIO_ODR_ODR_7;
            MOTOR_PORT_B->ODR &= ~GPIO_ODR_ODR_0;
            TIM3->CCR1 = 0;
            break;

        case 1:
            /* Quat chay toc do trung binh: Bat STBY, cau hinh quay thuan (AIN1=1, AIN2=0) va duty 50% */
            MOTOR_PORT_B->ODR |= GPIO_ODR_ODR_1;  /* STBY = 1 */
            MOTOR_PORT_A->ODR |= GPIO_ODR_ODR_7;  /* AIN1 = 1 */
            MOTOR_PORT_B->ODR &= ~GPIO_ODR_ODR_0; /* AIN2 = 0 */
            TIM3->CCR1 = 10000;                    /* Duty cycle 50% (10000/20000) */
            break;

        case 2:
            /* Quat chay toc do toi da: Bat STBY, cau hinh quay thuan (AIN1=1, AIN2=0) va duty 100% */
            MOTOR_PORT_B->ODR |= GPIO_ODR_ODR_1;  /* STBY = 1 */
            MOTOR_PORT_A->ODR |= GPIO_ODR_ODR_7;  /* AIN1 = 1 */
            MOTOR_PORT_B->ODR &= ~GPIO_ODR_ODR_0; /* AIN2 = 0 */
            TIM3->CCR1 = 20000;                    /* Duty cycle 100% (20000/20000) */
            break;

        default:
            /* Truong hop loi hoac gia tri la: Tu dong tat quat de bao ve he thong */
            MOTOR_PORT_B->ODR &= ~GPIO_ODR_ODR_1;
            TIM3->CCR1 = 0;
            break;
    }
}

void Actuator_Set_Window_Position(uint8_t angle) {
    /* Gioi han goc xoay tu 0 den 90 do de bao ve co cau ha kinh */
    if (angle > 90) {
        angle = 90;
    }

    /* Cong thuc noi suy tuyen tinh de tinh do rong xung dieu khien servo SG90
     * Goc 0 do ung voi xung rong 1.0ms (CCR = 1000)
     * Goc 90 do ung voi xung rong 1.5ms (CCR = 1500)
     * CCR = 1000 + (angle * 500) / 90 */
    uint32_t pulse_width = 1000 + ((uint32_t)angle * 500) / 90;

    /* Cap nhat gia tri vao thanh ghi so sanh kenh 3 de thay doi goc servo */
    TIM3->CCR3 = pulse_width;
}
