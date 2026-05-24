/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Carlos Villarreal and CETYS Universidad are not liable for any
 * misuse of this material.
 *
 ******************************************************************************/
/**
 * @file serial.h
 * @brief High-level Serial module for STM32F411RE.
 *
 * Provides a simple API for terminal communication over USART2.
 * Internally uses the UART driver and Utils module.
 *
 * Usage:
 *   serial_init();
 *   serial_printf("ADC Value: %d | Voltage: %d mV\n", adc, voltage);
 *
 * @author Team 4
 * @date May 14, 2026
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Maximum length of a formatted serial message */
#define SERIAL_BUFFER_SIZE  128U

/*** Function Prototypes ***/

/**
 * @brief Initialize the Serial module.
 *
 * Calls uart_init() to configure USART2 at 115200-8N1.
 * Must be called before any serial_printf().
 */
void serial_init(void);

/**
 * @brief Format and transmit a string over USART2.
 *
 * Uses utils_snprintf internally to format the message into a local buffer,
 * then transmits it character by character via uart_sendString().
 *
 * Supported format specifiers: %d, %u, %x, %s, %c, %%
 *
 * @param format  Format string (same syntax as utils_snprintf).
 * @param ...     Variable arguments matching the format specifiers.
 */
void serial_printf(const char *format, ...);

#endif /* __SERIAL_H__ */
