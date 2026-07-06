#include "FreeRTOS.h"
#include <stdio.h>
#include "probe_config.h"
#include "tusb.h"
#include "task.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "power_monitor.h"
#include "cdc_uart.h"
#include <sys/types.h>
#include <pico/stdlib.h>
#include <string.h>
#include "pico/binary_info.h"
#include <math.h>

#define PICO_SPI_INSTANCE spi1
#define PICO_INA_MISO_PIN 8
#define PICO_INA_CS_PIN 9
#define PICO_INA_SCLK_PIN 10
#define PICO_INA_MOSI_PIN 11
#define PICO_INA_ALERT 12

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


// Most co nfig registers are 16 bits
// VSHUNT, VBUS, CURRENT, POWER are 24 bits
// ENERGY and CHARGE are 40 bits
uint8_t reset_setting[] = {0b10000000, 0b00000000};
uint8_t config_setting[] = {0b00000000, 0b01100000};
uint8_t adc_config_setting[] = {0b10100110, 0b11011000};
uint8_t alert_config_setting[] = {0b01000000, 0b00000000}; // Alert on current overflow

// 10ohm stunt
//uint8_t shunt_cal_setting[] = {0x09, 0xC4}; // max 0.01A
//uint8_t shunt_cal_setting[] = {0x30, 0xD4}; // max 0.05A
//uint8_t shunt_cal_setting[] = {0x61, 0xA8}; // max 0.1A

// 1.5ohm shunt
//uint8_t shunt_cal_setting[] = {0x01, 0x77}; // max 0.01A
uint8_t shunt_cal_setting[] = {0x07, 0x53}; // max 0.05A
//uint8_t shunt_cal_setting[] = {0x0E, 0xA6}; // max 0.1A
//uint8_t shunt_cal_setting[] = {0x3A, 0x98}; // max 0.1A adcrange=1

// Breakout board shunt 0.015ohm
//uint8_t shunt_cal_setting[] = {0x08, 0x00}; 

//const float current_lsb = 0.0190735; // max 0.01
const float current_lsb = 0.095367; // max 0.05
//const float current_lsb = 0.190735; // max 0.1
//const float current_lsb = 2.604; // uA per bit

volatile int current = 0;
volatile int times = 0;
volatile int err = 0;

int write_spi_device(uint8_t reg, uint8_t *buf, size_t len) {
    gpio_put(PICO_INA_CS_PIN, 0);
    uint8_t data[len + 1];
    data[0] = reg << 2;
    memcpy(&data[1], buf, len);
    int err = spi_write_blocking(PICO_SPI_INSTANCE, data, len + 1);
    gpio_put(PICO_INA_CS_PIN, 1);
    if (err < 0) {
        char err_buf[100];
        sprintf(err_buf, "SPI write error: %d\r\n", err);
        tud_cdc_write(err_buf, strlen(err_buf));
        return 1;
    }
    return 0;
}

int spi_power_monitor_init(){
    gpio_init(PICO_INA_CS_PIN);
    gpio_set_dir(PICO_INA_CS_PIN, GPIO_OUT);
    gpio_put(PICO_INA_CS_PIN, 1);
    spi_init(PICO_SPI_INSTANCE, 5000000);
    gpio_pull_up(5);
    spi_set_format(PICO_SPI_INSTANCE, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(PICO_INA_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_INA_SCLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_INA_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_pull_up(PICO_INA_MISO_PIN);
    gpio_pull_up(PICO_INA_SCLK_PIN);
    bi_decl(bi_3pins_with_func(PICO_INA_MISO_PIN, PICO_INA_MOSI_PIN, PICO_INA_SCLK_PIN, GPIO_FUNC_SPI));
    
    write_spi_device(CONFIG_ADDR, reset_setting, sizeof(reset_setting));
    write_spi_device(CONFIG_ADDR, config_setting, sizeof(config_setting));
    write_spi_device(ADC_CONFIG_ADDR, adc_config_setting, sizeof(adc_config_setting));
    write_spi_device(SHUNT_CAL_ADDR, shunt_cal_setting, sizeof(shunt_cal_setting));
    write_spi_device(DIAG_ALRT_ADDR, alert_config_setting, sizeof(alert_config_setting));
    return 0;
}

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

int i2c_power_monitor_init(){
    i2c_init(PICO_I2C_INSTANCE, 1000 * 1000);
    gpio_set_function(PICO_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_I2C_SDA_PIN);
    gpio_pull_up(PICO_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_I2C_SDA_PIN, PICO_I2C_SCL_PIN, GPIO_FUNC_I2C));
    
    write_i2c_device(CONFIG_ADDR, reset_setting, sizeof(reset_setting));
    write_i2c_device(CONFIG_ADDR, config_setting, sizeof(config_setting));
    write_i2c_device(ADC_CONFIG_ADDR, adc_config_setting, sizeof(adc_config_setting));
    write_i2c_device(SHUNT_CAL_ADDR, shunt_cal_setting, sizeof(shunt_cal_setting));
    write_i2c_device(DIAG_ALRT_ADDR, alert_config_setting, sizeof(alert_config_setting));
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

int read_spi_device(uint8_t reg, uint8_t *buf, size_t len) {
    uint8_t tx_buf[5] = {0};
    gpio_put(PICO_INA_CS_PIN, 0);
    tx_buf[0] = (reg << 2) | 0x01; 
    uint8_t rx_buf[5] = {0};
    int err = spi_write_read_blocking(PICO_SPI_INSTANCE, tx_buf, rx_buf, len + 1);
    gpio_put(PICO_INA_CS_PIN, 1);
    if (err < 0) {
        char err_buf[100];
        sprintf(err_buf, "SPI write/read error: %d\r\n", err);
        tud_cdc_write(err_buf, strlen(err_buf));
        return 1;
    }
    memcpy(buf, &rx_buf[1], len);  // Read and write happen simultaneously so first byte in read buffer is garbage
    return 0;
}

const uint8_t SYNC_BYTES[2] = {0xAA, 0x55};
static TaskHandle_t spi_task_handle = NULL;
void alert_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(spi_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void power_monitor_thread(void *ptr) {
    spi_task_handle = xTaskGetCurrentTaskHandle();
    spi_power_monitor_init();
    gpio_init(PICO_INA_ALERT);
    gpio_pull_up(PICO_INA_ALERT);
    uint8_t buf[5] = {0};
    gpio_set_irq_enabled_with_callback(PICO_INA_ALERT, GPIO_IRQ_EDGE_FALL, true, &alert_callback);
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        read_spi_device(DIAG_ALRT_ADDR, buf, 2);
        read_spi_device(CURRENT_ADDR, buf, 3);
        uint32_t raw_current = ((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));
        if (raw_current & 0x80000) { 
            raw_current |= 0xFFF00000; // Sign extend if negative 
        }
        int32_t current_reading = (int32_t)raw_current;
        current += current_reading;
        times++;
    }
}




void print_power_monitor_info_thread(void *ptr) {
    while (1) {      
        if (times == 0 || current == 0) {
            continue;
        }
        float val = (current * current_lsb)/times; 
        tud_cdc_write(SYNC_BYTES, sizeof(SYNC_BYTES));
        tud_cdc_write((uint8_t*)&val, sizeof(val));
        current = 0;
        times = 0;
    }
}