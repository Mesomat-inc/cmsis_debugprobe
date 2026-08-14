#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

extern TaskHandle_t spi_taskhandle;

// UART logs
#define LOG_UART_TX 4
#define LOG_UART_RX 5
#define LOG_UART_INTERFACE uart1
#define LOG_UART_BAUDRATE 115200

int power_monitor_init(void);
void power_monitor_thread(void *ptr);
void print_power_monitor_info_thread(void *ptr);


void uart_log_init(void);
void uart_log_thread(void *ptr);
#endif