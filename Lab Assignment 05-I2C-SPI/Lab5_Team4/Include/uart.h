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
 * @file uart.h
 * @brief UART driver for STM32F411RE using USART2.
 *
 * USART_TypeDef, USART2, and USART2_BASE are provided by stm32f411xe.h
 * (included via stm32f4xx.h). This header only defines the driver-specific
 * bit masks, constants, and function prototypes.
 *
 * Configures USART2 on PA2 (TX) / PA3 (RX) with alternate function AF7.
 * Fixed configuration: 115200-8N1.
 *
 * Physical connection on Nucleo-64:
 *   PA2 -> USART2_TX -> ST-Link Virtual COM Port
 *   PA3 -> USART2_RX -> ST-Link Virtual COM Port
 *
 * Baud rate configuration:
 *   BRR = fAPB1 / baud = 16,000,000 / 115,200 = 0x008B
 *   Mantissa = 8, Fraction = 11 -> BRR = (8 << 4) | 11 = 0x008B
 *
 * Non-Functional Requirements compliance:
 *   NFR-1 (Lightweight): direct register access, no HAL, no dynamic memory.
 *   NFR-2 (Real-time):   single-write CR1 init, tight TXE polling loop.
 *
 * @author Team 4
 * @date May 14, 2026
 */

#ifndef __UART_H__
#define __UART_H__

/*** Includes ***/
#include <stdint.h>
#include "stm32f4xx.h"   /* Provides USART_TypeDef, USART2, USART2_BASE, RCC */

/*** Preprocessor Definitions ***/

/* RCC_APB1ENR bit to enable USART2 clock */
#define USART2EN      (1U << 17)

/* Alternate function number for USART2 on PA2/PA3 */
#define AF7_USART2    7U

/* Status Register (SR) bits */
#define SR_TXE   (1U << 7)   /* TX Data Register Empty */
#define SR_TC    (1U << 6)   /* Transmission Complete   */
#define SR_RXNE  (1U << 5)   /* RX Not Empty            */

/* Control Register 1 (CR1) bits */
#define CR1_UE   (1U << 13)  /* USART Enable       */
#define CR1_TE   (1U << 3)   /* Transmitter Enable */
#define CR1_RE   (1U << 2)   /* Receiver Enable    */

/* Control Register 2 (CR2) stop-bits mask */
#define CR2_STOP (3U << 12)

/* BRR value for 115200 bps at 16 MHz (oversampling x16) */
#define UART_BRR_115200  0x008BU

/*** Enumerations ***/

typedef enum {
    UART_OK    = 0,
    UART_ERROR = 1
} UART_Status_t;

/*** Function Prototypes ***/

/**
 * @brief Initialize USART2 peripheral.
 *
 * FR-1: Enables the clock for USART2.
 * FR-2: Configures the baud rate based on the system clock (115200 @ 16 MHz).
 * FR-3: Enables the transmitter and the peripheral in a single CR1 write.
 * NFR-1: No overhead — direct register writes only.
 * NFR-2: Single atomic CR1 write avoids transient intermediate states.
 */
void uart_init(void);

/**
 * @brief Transmit a single byte over USART2.
 *
 * FR-4: Waits until the Transmit Data Register is empty (TXE flag).
 * FR-5: Writes the 8-bit character to the data register.
 * NFR-2: Tight polling loop — bounded by hardware TX time (~87 us at 115200).
 *
 * @param data Byte to transmit.
 */
void uart_write(char data);

/**
 * @brief Transmit a null-terminated string over USART2.
 *
 * Extension (not in SRS). Calls uart_write() per character.
 * NFR-1: No buffer copies — streams directly from source pointer.
 *
 * @param str Pointer to the null-terminated string.
 */
void uart_sendString(const char *str);

/**
 * @brief Receive a single byte from USART2.
 *
 * Extension (not in SRS). Blocks until RXNE flag is set.
 *
 * @return Received character.
 */
char uart_receiveChar(void);

#endif /* __UART_H__ */
