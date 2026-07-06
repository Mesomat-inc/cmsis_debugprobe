#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

extern TaskHandle_t spi_taskhandle;

int power_monitor_init(void);
void power_monitor_thread(void *ptr);

void print_power_monitor_info_thread(void *ptr);
#endif