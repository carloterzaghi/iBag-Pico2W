#include "mpu6050.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Configuração I2C
#define I2C_PORT i2c0
#define I2C_SDA_PIN 20
#define I2C_SCL_PIN 21
#define I2C_FREQ 400000  // 400 kHz

// Thresholds para detecção de virada brusca
#define ACCEL_THRESHOLD 20000   // Threshold para aceleração (valores brutos)
#define GYRO_THRESHOLD 15000    // Threshold para giroscópio (valores brutos)
#define CALIBRATION_TIME_MS 10000  // 10 segundos para calibração

// Variável estática para rastrear se houve shake
static bool shake_detected = false;

// Variáveis para calibração
static bool is_calibrating = false;
static uint32_t calibration_start_time = 0;
static bool is_calibrated = false;
static mpu6050_accel_t baseline_accel = {0, 0, 0};
static mpu6050_gyro_t baseline_gyro = {0, 0, 0};

// Leitura de registrador do MPU6050
static bool mpu6050_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    int ret = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    if (ret < 0) return false;
    
    ret = i2c_read_blocking(I2C_PORT, MPU6050_ADDR, data, len, false);
    return ret >= 0;
}

// Escrita em registrador do MPU6050
static bool mpu6050_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    int ret = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    return ret >= 0;
}

// Inicializar o MPU6050
bool mpu6050_init(void) {
    // Inicializar I2C
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    sleep_ms(100);  // Dar tempo para o MPU6050 inicializar
    
    // Verificar WHO_AM_I
    uint8_t who_am_i;
    if (!mpu6050_read_reg(MPU6050_WHO_AM_I, &who_am_i, 1)) {
        printf("MPU6050: Falha ao ler WHO_AM_I\n");
        return false;
    }
    
    printf("MPU6050: WHO_AM_I = 0x%02X\n", who_am_i);
    
    // Acordar o MPU6050 (desabilitar sleep mode)
    if (!mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00)) {
        printf("MPU6050: Falha ao acordar dispositivo\n");
        return false;
    }
    
    sleep_ms(100);
    
    printf("MPU6050 inicializado com sucesso!\n");
    printf("  - I2C0: SDA=GPIO%d, SCL=GPIO%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    printf("  - Detecção de virada brusca: ATIVA\n\n");
    
    return true;
}

// Ler dados do acelerômetro
bool mpu6050_read_accel(mpu6050_accel_t *accel) {
    uint8_t data[6];
    
    if (!mpu6050_read_reg(MPU6050_ACCEL_XOUT_H, data, 6)) {
        return false;
    }
    
    accel->x = (int16_t)((data[0] << 8) | data[1]);
    accel->y = (int16_t)((data[2] << 8) | data[3]);
    accel->z = (int16_t)((data[4] << 8) | data[5]);
    
    return true;
}

// Ler dados do giroscópio
bool mpu6050_read_gyro(mpu6050_gyro_t *gyro) {
    uint8_t data[6];
    
    if (!mpu6050_read_reg(MPU6050_GYRO_XOUT_H, data, 6)) {
        return false;
    }
    
    gyro->x = (int16_t)((data[0] << 8) | data[1]);
    gyro->y = (int16_t)((data[2] << 8) | data[3]);
    gyro->z = (int16_t)((data[4] << 8) | data[5]);
    
    return true;
}

// Detectar virada brusca
bool mpu6050_detect_shake(void) {
    // Se já detectou shake, retornar true
    if (shake_detected) {
        return true;
    }
    
    // Se ainda está calibrando, não detectar shake
    if (is_calibrating || !is_calibrated) {
        return false;
    }
    
    mpu6050_accel_t accel;
    mpu6050_gyro_t gyro;
    
    // Ler dados do sensor
    if (!mpu6050_read_accel(&accel) || !mpu6050_read_gyro(&gyro)) {
        return false;
    }
    
    // Calcular diferença em relação à linha base (posição de referência)
    int32_t accel_diff = abs(accel.x - baseline_accel.x) + 
                         abs(accel.y - baseline_accel.y) + 
                         abs(accel.z - baseline_accel.z);
    
    int32_t gyro_diff = abs(gyro.x - baseline_gyro.x) + 
                        abs(gyro.y - baseline_gyro.y) + 
                        abs(gyro.z - baseline_gyro.z);
    
    // Log detalhado dos valores do MPU6050
    printf("[MPU6050] Accel: X=%6d Y=%6d Z=%6d | Gyro: X=%6d Y=%6d Z=%6d | Diff: A=%6ld G=%6ld\n",
           accel.x, accel.y, accel.z, gyro.x, gyro.y, gyro.z, accel_diff, gyro_diff);
    
    // Detectar virada brusca se a diferença exceder threshold
    if (accel_diff > ACCEL_THRESHOLD || gyro_diff > GYRO_THRESHOLD) {
        shake_detected = true;
        printf("\n⚠️  VIRADA BRUSCA DETECTADA!\n");
        printf("   Valores Atuais:\n");
        printf("     Accel: X=%d, Y=%d, Z=%d\n", accel.x, accel.y, accel.z);
        printf("     Gyro:  X=%d, Y=%d, Z=%d\n", gyro.x, gyro.y, gyro.z);
        printf("   Baseline (referência):\n");
        printf("     Accel: X=%d, Y=%d, Z=%d\n", baseline_accel.x, baseline_accel.y, baseline_accel.z);
        printf("     Gyro:  X=%d, Y=%d, Z=%d\n", baseline_gyro.x, baseline_gyro.y, baseline_gyro.z);
        printf("   Diferenças:\n");
        printf("     Accel diff: %ld (threshold: %d)\n", accel_diff, ACCEL_THRESHOLD);
        printf("     Gyro diff:  %ld (threshold: %d)\n\n", gyro_diff, GYRO_THRESHOLD);
        return true;
    }
    
    return false;
}

// Resetar detecção de shake e iniciar calibração
void mpu6050_reset_shake_detection(void) {
    shake_detected = false;
    is_calibrating = true;
    is_calibrated = false;
    calibration_start_time = to_ms_since_boot(get_absolute_time());
    printf("MPU6050: Estado de virada resetado\n");
    printf("MPU6050: Iniciando calibração de 10 segundos...\n");
    printf("         NÃO MOVA O DISPOSITIVO!\n");
}

// Atualizar processo de calibração
void mpu6050_update_calibration(void) {
    if (!is_calibrating) {
        return;
    }
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = current_time - calibration_start_time;
    
    if (elapsed >= CALIBRATION_TIME_MS) {
        // Finalizar calibração - ler valores base
        mpu6050_accel_t accel;
        mpu6050_gyro_t gyro;
        
        if (mpu6050_read_accel(&accel) && mpu6050_read_gyro(&gyro)) {
            baseline_accel = accel;
            baseline_gyro = gyro;
            is_calibrated = true;
            is_calibrating = false;
            
            printf("\n✅ MPU6050: Calibração completa!\n");
            printf("   Baseline Accel: X=%d, Y=%d, Z=%d\n", 
                   baseline_accel.x, baseline_accel.y, baseline_accel.z);
            printf("   Baseline Gyro:  X=%d, Y=%d, Z=%d\n\n", 
                   baseline_gyro.x, baseline_gyro.y, baseline_gyro.z);
        }
    }
}
