#ifndef BOARD_MULTIPROBE_H_
#define BOARD_MULTIPROBE_H_

#define PROBE_IO_RAW
#define PROBE_CDC_UART

// Debug probe config
#define UART_CONFIG0                                                                                                                                   \
  {.index = 0, .name = "IFACE0", .txPin = 4, .rxPin = 5, .hwUartId = 1, .baud = 115200,                                                             \} \
  // [debug 1]

// Debug probe config
#define PROBE_CONFIG0                                                          \
  {.name = "IFACE0",                                                           \
   .pio = pio0,                                                                \
   .sm = 0,                                                                    \
   .clkPin = 2,                                                                \
   .dioPin = 3,                                                                \
   .activityLedPin = 18} // [debug 1]
#define PROBE_CONFIG1                                                          \
  {                                                                            \
      .name = "IFACE1",                                                        \
      .pio = pio0,                                                             \
      .sm = 1,                                                                 \
      .clkPin = 9,                                                             \
      .dioPin = 10,                                                            \
  } // [debug 2]

#define PROBE_CONFIG2                                                          \
  {                                                                            \
      .name = "IFACE2",                                                        \
      .pio = pio1,                                                             \
      .sm = 2,                                                                 \
      .clkPin = 24,                                                            \
      .dioPin = 25,                                                            \
  } // [debug 3]
#define PROBE_PRODUCT_STRING "Multiprobe on Pico (CMSIS-DAP)"

#endif