/**
 ******************************************************************************
 * @file    spi_driver.h
 * @author  Team 4
 * @brief   SPI Driver — Public API
 *          STM32F411RE, SPI1, Master Mode, 8-bit, Mode 0 (CPOL=0, CPHA=0)
 *          PA5 = SCK   PA6 = MISO   PA7 = MOSI   PA4 = CS   (AF5)
 ******************************************************************************
 */
#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "stm32f4xx.h"
#include "gpio_driver.h"
#include <stdint.h>
#include <stddef.h>

/* ── Pin and alternate function configuration ──────────────────────────────*/
#define SPI_SCK_PORT        GPIO_PORT_A
#define SPI_SCK_PIN         5U
#define SPI_MISO_PORT       GPIO_PORT_A
#define SPI_MISO_PIN        6U
#define SPI_MOSI_PORT       GPIO_PORT_A
#define SPI_MOSI_PIN        7U
#define SPI_CS_PORT         GPIO_PORT_A
#define SPI_CS_PIN          4U
#define SPI_AF              5U          /**< Alternate function 5 = SPI1      */

/* ── SPI clock configuration (SPI1 on APB2, 16 MHz system clock) ──────────*/
/**
 * @brief Baud rate prescaler: fPCLK/8 = 16 MHz/8 = 2 MHz.
 *        BR[2:0] = 010 → bits [5:3] of CR1.
 *        Suitable for most sensors including the ADXL345 (max 5 MHz).
 */
#define SPI_CR1_BR_DIV8     (0x2U << 3U)

/**
 * @brief Timeout in CPU cycles for flag polling (~10 ms at 16 MHz).
 *        If a flag is not set within this window, the operation is aborted.
 */
#define SPI_TIMEOUT         160000U

/* ── Error flags in SR ─────────────────────────────────────────────────────*/
/**
 * @brief Mask covering all hardware error flags in SR:
 *        OVR (overrun), MODF (mode fault), CRCERR (CRC error).
 */
#define SPI_ERROR_FLAGS     (SPI_SR_OVR | SPI_SR_MODF | SPI_SR_CRCERR)

/* ── Return status ──────────────────────────────────────────────────────── */
typedef enum
{
    SPI_OK   = 0,   /**< Transaction completed successfully                  */
    SPI_ERR  = 1,   /**< Bus or peripheral error occurred                    */
    SPI_BUSY = 2    /**< Bus is busy and could not be acquired               */
} SPI_Status_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initializes the SPI1 peripheral in Master Mode.
 *         Configuration: 8-bit data frame, Mode 0 (CPOL=0, CPHA=0),
 *         MSB first, software slave management (SSM=1, SSI=1),
 *         baud rate = fPCLK/8 (~2 MHz).
 *         Configures GPIO pins:
 *           PA5 (SCK), PA6 (MISO), PA7 (MOSI) as AF5 push-pull.
 *           PA4 (CS) as GPIO output push-pull, initially HIGH (deselected).
 *         Enables RCC clocks for GPIOA and SPI1.
 */
void spi_init(void);

/**
 * @brief  Transmits multiple bytes over the SPI bus.
 *         Drives the SPI clock while sending each byte from the buffer.
 *         The received bytes during transmission are discarded.
 *         The CS line must be managed by the caller via spi_csEnable()
 *         and spi_csDisable().
 *
 * @param  data  Pointer to the transmit buffer (must not be NULL).
 * @param  len   Number of bytes to transmit (must be > 0).
 * @retval SPI_OK    All bytes transmitted successfully.
 * @retval SPI_ERR   Invalid arguments or bus/peripheral error.
 * @retval SPI_BUSY  Bus could not be acquired within timeout.
 */
SPI_Status_t spi_transmit(const uint8_t *data, uint32_t len);

/**
 * @brief  Receives multiple bytes over the SPI bus.
 *         Drives the SPI clock by sending dummy bytes (0x00) while
 *         capturing the incoming data into the receive buffer.
 *         The CS line must be managed by the caller via spi_csEnable()
 *         and spi_csDisable().
 *
 * @param  data  Pointer to the receive buffer (must not be NULL).
 * @param  len   Number of bytes to receive (must be > 0).
 * @retval SPI_OK    All bytes received successfully.
 * @retval SPI_ERR   Invalid arguments or bus/peripheral error.
 * @retval SPI_BUSY  Bus could not be acquired within timeout.
 */
SPI_Status_t spi_receive(uint8_t *data, uint32_t len);

/**
 * @brief  Drives the CS line LOW to select the target SPI device.
 *         Must be called before spi_transmit() or spi_receive().
 *         The caller is responsible for asserting CS before any transaction
 *         and releasing it with spi_csDisable() once done.
 */
void spi_csEnable(void);

/**
 * @brief  Drives the CS line HIGH to deselect the target SPI device.
 *         Must be called after the SPI transaction is complete.
 *         Ensures the bus is idle (BSY flag cleared) before releasing CS
 *         so the slave correctly latches the last transferred byte.
 */
void spi_csDisable(void);

#endif /* SPI_DRIVER_H */