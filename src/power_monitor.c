#include "FreeRTOS.h"
#include <stdio.h>
#include "probe_config.h"
#include "tusb.h"
#include "task.h"
#include "hardware/i2c.h"
#include "power_monitor.h"
#include "cdc_uart.h"
#include <sys/types.h>
#include <pico/stdlib.h>
#include <string.h>
#include "pico/binary_info.h"

#define PICO_I2C_INSTANCE i2c1
#define PICO_I2C_SDA_PIN 6
#define PICO_I2C_SCL_PIN 7  

#define I2C_ADDR 0x40

#define CONFIG_ADDR 0x00
#define ADC_CONFIG_ADDR 0x01
#define SHUNT_CAL_ADDR 0x02
#define SHUNT_TEMPCO_ADDR 0x03
#define VSHUNT_ADDR 0x04
#define VBUS_ADDR 0x05
#define DIETEMP_ADDR 0x06
#define CURRENT_ADDR 0x07
#define POWER_ADDR 0x08
#define ENERGY_ADDR 0x09
#define CHARGE_ADDR 0x0A
#define DIAG_ALRT_ADDR 0x0B
#define SOVL_ADDR 0x0C
#define SUVL_ADDR 0x0D
#define BOVL_ADDR 0x0E
#define BUVL_ADDR 0x0F
#define TEMP_LIMIT_ADDR 0x10
#define PWR_LIMIT_ADDR 0x11
#define MANUFACTURER_ID_ADDR 0x3E
#define DEVICE_ID_ADDR 0x3F

const float current_lsb = 2.604; // uA per bit

// Most config registers are 16 bits
// VSHUNT, VBUS, CURRENT, POWER are 24 bits
// ENERGY and CHARGE are 40 bits

uint8_t config_setting[] = {0b00000000, 0b10011000};
uint8_t adc_config_setting[] = {0xFB, 0x68};
uint8_t shunt_cal_setting[] = {0x08, 0x00}; 

int current = 0;
int current_peak = 0;
int power = 0;
int voltage = 0;
int times = 0;
int err = 0;
// Unused for now 
// uint8_t diag_alrt_setting[] = {}

int write_i2c_device(uint8_t reg, uint8_t *buf, size_t len) {
    uint8_t data[len + 1];
    data[0] = reg;
    memcpy(data + 1, buf, len);
    int err = i2c_write_timeout_us(PICO_I2C_INSTANCE, I2C_ADDR, data, len + 1, false, 10000);
    if (err < 0) {
        char err_buf[100];
        sprintf(err_buf, "I2C write error: %d\r\n", err);
        tud_cdc_write(err_buf, strlen(err_buf));
        return 1;
    }
    return 0;
}

int power_monitor_init(){
    i2c_init(PICO_I2C_INSTANCE, 1000 * 1000);
    gpio_set_function(PICO_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_I2C_SDA_PIN);
    gpio_pull_up(PICO_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_I2C_SDA_PIN, PICO_I2C_SCL_PIN, GPIO_FUNC_I2C));
    

    write_i2c_device(CONFIG_ADDR, config_setting, sizeof(config_setting));
    write_i2c_device(ADC_CONFIG_ADDR, adc_config_setting, sizeof(adc_config_setting));
    write_i2c_device(SHUNT_CAL_ADDR, shunt_cal_setting, sizeof(shunt_cal_setting));

    return 0;
}

int read_i2c_device(uint8_t reg, uint8_t *buf, size_t len) {
    int err = i2c_write_timeout_us(PICO_I2C_INSTANCE, I2C_ADDR, &reg, 1, true, 10000);
    if (err < 0) {
        char err_buf[100];
        sprintf(err_buf, "I2C write addr error: %d\r\n", err);
        tud_cdc_write(err_buf, strlen(err_buf));
        return 1;
    }
    err = i2c_read_timeout_us(PICO_I2C_INSTANCE, I2C_ADDR, buf, len, false, 10000);
    if (err < 0) {
        char err_buf[100];
        sprintf(err_buf, "I2C read error: %d\r\n", err);
        tud_cdc_write(err_buf, strlen(err_buf));
        return 2;
    }
    return 0;
}

void power_monitor_thread(void *ptr) {
    power_monitor_init();
    uint8_t buf[5] = {0};
    while (1) {
        read_i2c_device(CURRENT_ADDR, buf, 3);
        uint32_t raw_current = ((buf[0] << 16) | (buf[1] << 8) | buf[2]) >> 4;
        int32_t current_reading = (int32_t)raw_current;
        if (current_reading & 0x80000) { 
            current_reading |= 0xFFF00000; 
        }
        current += current_reading;
        if (current_reading > current_peak) {
            current_peak = current_reading;
        }
        read_i2c_device(POWER_ADDR, buf, 3);
        uint32_t raw_power = ((buf[0] << 16) | (buf[1] << 8) | buf[2]);
        int32_t power_reading = (int32_t)raw_power;
        if (power_reading & 0x80000) { 
            power_reading |= 0xFFF00000; 
        }
        power += power_reading; 
        read_i2c_device(VBUS_ADDR, buf, 3);
        voltage += ((buf[0] << 16) | (buf[1] << 8) | buf[2]) >> 4;
        times++;
    }
}

void print_power_monitor_info_thread(void *ptr) {
    while (1) {
        char output[150];
        if (times == 0 || current == 0){
            continue;
        }
        int len = snprintf(output, sizeof(output), "%lf\r\n", (current * current_lsb) / times);
        tud_cdc_write(output, len);
        current = 0;
        power = 0;
        voltage = 0;
        times = 0;
        current_peak = 0;
    }
}

