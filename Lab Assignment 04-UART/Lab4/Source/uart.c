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
 * @file uart.c
 * @brief UART driver implementation for STM32F411RE.
 *
 * Non-Functional Requirements compliance:
 *   NFR-1 (Lightweight and efficient):
 *     - Direct register access only; no HAL or middleware dependencies.
 *     - CR1 configured in a single atomic write to avoid intermediate states.
 *     - No dynamic memory allocation.
 *   NFR-2 (Real-time performance):
 *     - uart_write() polls TXE with a tight busy-wait loop (no blocking calls).
 *     - uart_init() uses a single CR1 write, minimizing peripheral setup time.
 *     - All functions are O(1) or O(n) with no unbounded waits beyond hardware.
 *
 * @author Team 4
 * @date May 14, 2026
 */

/*** Includes ***/
#include "uart.h"
#include "gpio_driver.h"

/*** Function Definitions ***/

/**
 * @brief Initialize USART2 peripheral.
 *
 * FR-1: Enables the clock for USART2 (RCC->APB1ENR bit 17).
 * FR-2: Configures baud rate to 115200 bps at 16 MHz APB1.
 * FR-3: Enables the transmitter (TE) and the peripheral (UE).
 *
 * NFR-1/NFR-2: CR1 is configured in a single write to avoid transient
 * intermediate states that could affect real-time behavior.
 */
void uart_init(void)
{
    /* FR-1: Enable clocks via gpio_driver and direct RCC access */
    gpio_initPort(GPIO_PORT_A);       /* GPIOA clock */
    RCC->APB1ENR |= USART2EN;        /* USART2 clock */

    /* Configure PA2 (TX) and PA3 (RX) as alternate function AF7 (USART2) */
    GPIO_PinCfg_t cfg = {
        .mode  = GPIO_MODE_ALT_FN,
        .pull  = GPIO_PULL_NONE,
        .otype = GPIO_OTYPE_PUSH_PULL,
        .speed = GPIO_SPEED_HIGH
    };
    gpio_setPinMode(GPIO_PORT_A, 2U, &cfg);
    gpio_setAlternateFunction(GPIO_PORT_A, 2U, AF7_USART2);

    gpio_setPinMode(GPIO_PORT_A, 3U, &cfg);
    gpio_setAlternateFunction(GPIO_PORT_A, 3U, AF7_USART2);

    /* FR-2: Set baud rate to 115200 bps at 16 MHz APB1 */
    USART2->BRR = UART_BRR_115200;

    /* CR2: 1 stop bit (bits [13:12] = 00) */
    USART2->CR2 &= ~CR2_STOP;

    /* FR-3 + NFR-1/NFR-2: Enable TX, RX and USART in a single atomic write.
     * Avoids intermediate states where only TE or UE is set. */
    USART2->CR1 = CR1_TE | CR1_RE | CR1_UE;
}

/**
 * @brief Transmit a single byte over USART2.
 *
 * FR-4: Waits until the Transmit Data Register is empty (TXE flag).
 * FR-5: Writes the 8-bit character to the data register.
 *
 * NFR-2: Uses a tight polling loop on TXE — deterministic and
 * bounded by the hardware transmission time at 115200 bps (~87 us/byte).
 *
 * @param data Byte to transmit.
 */
void uart_write(char data)
{
    /* FR-4: Wait until TX Data Register is empty (TXE flag) */
    while (!(USART2->SR & SR_TXE)) {}

    /* FR-5: Write the byte to the data register */
    USART2->DR = (uint32_t)(data & 0xFFU);
}

/**
 * @brief Transmit a null-terminated string over USART2.
 *
 * Extension (not in SRS): helper that calls uart_write() per character.
 * NFR-1: No buffer copies — streams directly from the source string.
 *
 * @param str Pointer to the null-terminated string to transmit.
 */
void uart_sendString(const char *str)
{
    while (*str != '\0')
    {
        uart_write(*str++);
    }
}

/**
 * @brief Receive a single byte from USART2.
 *
 * Extension (not in SRS): blocks until RXNE flag is set.
 *
 * @return Received character.
 */
char uart_receiveChar(void)
{
    while (!(USART2->SR & SR_RXNE)) {}
    return (char)(USART2->DR & 0xFFU);
}
