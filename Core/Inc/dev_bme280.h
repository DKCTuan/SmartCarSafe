#ifndef DEV_BME280_H
#define DEV_BME280_H

#include <stdint.h>

/* Địa chỉ I2C của BME280 */
#define BME280_ADDR 0x76

/* Khởi tạo cảm biến (Gửi lệnh đọc Chip ID, lấy thông số bù nhiệt) */
void BME280_Init(void);

/* Hàm đọc và trả về giá trị nhiệt độ từ cảm biến BME280 */
float BME280_Read_Temperature(void);

#endif /* DEV_BME280_H */
