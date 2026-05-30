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
 * @file serial.c
 * @brief High-level Serial module implementation for STM32F411RE.
 *
 * @author Team 4
 * @date May 14, 2026
 */

/*** Includes ***/
#include "serial.h"
#include "uart.h"
#include "utils.h"
#include <stdarg.h>

/*** Local Variables ***/

/* Static buffer used internally by serial_printf */
static char serial_buffer[SERIAL_BUFFER_SIZE];

/*** Function Definitions ***/

/**
 * @brief Initialize the Serial module.
 *
 * Delegates to uart_init() which configures USART2 and the GPIO pins.
 */
void serial_init(void)
{
    uart_init();
}

/**
 * @brief Format and send a message over USART2.
 *
 * Steps:
 *   1. Build the formatted string in serial_buffer using utils_snprintf
 *      logic inline via va_list (utils_snprintf is not va_list-aware).
 *   2. Null-terminate the buffer.
 *   3. Transmit via uart_sendString().
 *
 * Supported specifiers: %d, %u, %x, %s, %c, %%
 *
 * @param format  Format string.
 * @param ...     Arguments matching the format specifiers.
 */
void serial_printf(const char *format, ...)
{
    va_list     args;
    char       *dst = serial_buffer;
    const char *fmt = format;

    va_start(args, format);

    while (*fmt != '\0')
    {
        if (*fmt == '%')
        {
            fmt++;
            switch (*fmt)
            {
                case 's':
                {
                    /* String: copy directly into buffer */
                    char *s = va_arg(args, char *);
                    while (*s != '\0') { *dst++ = *s++; }
                    break;
                }
                case 'd':
                {
                    /* Signed decimal integer */
                    uint8_t  tmp[33U];
                    uint32_t len = utils_itoa((int32_t)va_arg(args, int), tmp, 1U, 10U);
                    uint32_t i;
                    for (i = 0U; i < len; i++) { *dst++ = (char)tmp[i]; }
                    break;
                }
                case 'u':
                {
                    /* Unsigned decimal integer */
                    uint8_t  tmp[33U];
                    uint32_t len = utils_itoa((int32_t)va_arg(args, unsigned int), tmp, 0U, 10U);
                    uint32_t i;
                    for (i = 0U; i < len; i++) { *dst++ = (char)tmp[i]; }
                    break;
                }
                case 'x':
                {
                    /* Unsigned hexadecimal integer */
                    uint8_t  tmp[33U];
                    uint32_t len = utils_itoa((int32_t)va_arg(args, unsigned int), tmp, 0U, 16U);
                    uint32_t i;
                    for (i = 0U; i < len; i++) { *dst++ = (char)tmp[i]; }
                    break;
                }
                case 'c':
                    /* Single character */
                    *dst++ = (char)va_arg(args, int);
                    break;
                case '%':
                    *dst++ = '%';
                    break;
                default:
                    *dst++ = '%';
                    *dst++ = *fmt;
                    break;
            }
        }
        else
        {
            *dst++ = *fmt;
        }
        fmt++;
    }

    *dst = '\0';   /* Null-terminate the buffer */

    va_end(args);

    /* Transmit the formatted string over USART2 */
    uart_sendString(serial_buffer);
}
