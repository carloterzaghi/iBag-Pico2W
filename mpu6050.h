#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>

// Endereço I2C do MPU6050
#define MPU6050_ADDR 0x68

// Registradores do MPU6050
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_WHO_AM_I     0x75

// Estrutura para dados do acelerômetro
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050_accel_t;

// Estrutura para dados do giroscópio
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050_gyro_t;

// Funções públicas
bool mpu6050_init(void);
bool mpu6050_read_accel(mpu6050_accel_t *accel);
bool mpu6050_read_gyro(mpu6050_gyro_t *gyro);
bool mpu6050_detect_shake(void);
void mpu6050_reset_shake_detection(void);
void mpu6050_update_calibration(void);  // Atualizar processo de calibração

#endif // MPU6050_H
