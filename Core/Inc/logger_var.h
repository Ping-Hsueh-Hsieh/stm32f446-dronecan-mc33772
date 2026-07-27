#ifndef LOGGER_VAR_H_
#define LOGGER_VAR_H_

#include <stdint.h>

extern volatile uint8_t host_open;
extern volatile uint8_t header_sent;
extern volatile uint32_t host_open_tick;
extern volatile uint8_t host_open_pending;
#endif    // LOGGER_VAR_H_
