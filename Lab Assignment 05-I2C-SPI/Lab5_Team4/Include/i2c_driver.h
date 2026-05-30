/**
 ******************************************************************************
 * @file    i2c_driver.h
 * @author  Team 4
 * @brief   I2C Driver — Public API
 *          STM32F411RE, I2C1, Standard Mode 100 kHz
 *          PB8 = SCL   PB9 = SDA   (AF4, open-drain)
 **/
#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "stm32f4xx.h"
#include "gpio_driver.h"
#include <stdint.h>
#include <stddef.h>

/* ── Pin and alternate function configuration ──────────────────────────────*/
#define I2C_SCL_PORT        GPIO_PORT_B
#define I2C_SCL_PIN         8U
#define I2C_SDA_PORT        GPIO_PORT_B
#define I2C_SDA_PIN         9U
#define I2C_AF              4U

/* ── I2C timing constants (Standard Mode, 100 kHz, 16 MHz APB1 clock) ──── */
#define I2C_CR2_FREQ_MHZ    16U     /**< APB1 peripheral clock in MHz       */
#define I2C_CCR_SM_100K     80U     /**< Clock control register value        */
#define I2C_TRISE_SM        17U     /**< Maximum rise time register value    */

/**
 * @brief Timeout in CPU cycles for flag polling (~10 ms at 16 MHz).
 *        If a flag is not set within this window, the operation is aborted.
 */
#define I2C_TIMEOUT         160000U

/**
 * @brief Valid 7-bit I2C address range.
 *        Addresses 0x00–0x07 and 0x78–0x7F are reserved by the I2C spec.
 */
#define I2C_ADDR_MIN        0x08U
#define I2C_ADDR_MAX        0x77U

/* ── Return status ──────────────────────────────────────────────────────── */
typedef enum
{
    I2C_OK   = 0,   /**< Transaction completed successfully                  */
    I2C_ERR  = 1,   /**< Bus or peripheral error occurred                    */
    I2C_BUSY = 2    /**< Bus is busy and could not be acquired               */
} I2C_Status_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initializes the I2C1 peripheral in Standard Mode at 100 kHz.
 *         Configures GPIO pins PB8 (SCL) and PB9 (SDA) as AF4, open-drain.
 *         Enables RCC clocks for GPIOB and I2C1.
 *         Performs a software reset before configuration.
 */
void i2c_init(void);

/**
 * @brief  Recovers a hung I2C bus.
 *         Clocks SCL up to 9 times to release a slave stuck mid-byte,
 *         generates a manual STOP condition, resets the I2C peripheral,
 *         and re-initializes it via i2c_init().
 */
void i2c_recover(void);

/**
 * @brief  Writes data to a specific internal register of an I2C device.
 *         Transaction sequence: START → ADDR(W) → REG → DATA[0..n] → STOP
 *
 * @param  dev_addr  7-bit device address (must be in range 0x08–0x77).
 * @param  reg_addr  Target register address inside the device.
 * @param  data      Pointer to the data buffer to transmit.
 * @param  len       Number of bytes to transmit (must be > 0).
 * @retval I2C_OK    Transaction completed successfully.
 * @retval I2C_ERR   Invalid arguments, or bus/peripheral error.
 * @retval I2C_BUSY  Bus could not be acquired within timeout.
 */
I2C_Status_t i2c_writeRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                 const uint8_t *data, uint32_t len);

/**
 * @brief  Writes data directly to an I2C device (no register address phase).
 *         Transaction sequence: START → ADDR(W) → DATA[0..n] → STOP
 *
 * @param  dev_addr  7-bit device address (must be in range 0x08–0x77).
 * @param  data      Pointer to the data buffer to transmit.
 * @param  len       Number of bytes to transmit (must be > 0).
 * @retval I2C_OK    Transaction completed successfully.
 * @retval I2C_ERR   Invalid arguments, or bus/peripheral error.
 * @retval I2C_BUSY  Bus could not be acquired within timeout.
 */
I2C_Status_t i2c_writeDevice(uint8_t dev_addr,
                              const uint8_t *data, uint32_t len);

/**
 * @brief  Reads data from a specific internal register of an I2C device.
 *         Transaction sequence:
 *         START → ADDR(W) → REG → REPEATED START → ADDR(R) → DATA[0..n] → STOP
 *
 * @param  dev_addr  7-bit device address (must be in range 0x08–0x77).
 * @param  reg_addr  Source register address inside the device.
 * @param  data      Pointer to the receive buffer.
 * @param  len       Number of bytes to read (must be > 0).
 * @retval I2C_OK    Transaction completed successfully.
 * @retval I2C_ERR   Invalid arguments, or bus/peripheral error.
 * @retval I2C_BUSY  Bus could not be acquired within timeout.
 */
I2C_Status_t i2c_readRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                uint8_t *data, uint32_t len);

/**
 * @brief  Reads data directly from an I2C device (no register address phase).
 *         Transaction sequence: START → ADDR(R) → DATA[0..n] → STOP
 *
 * @param  dev_addr  7-bit device address (must be in range 0x08–0x77).
 * @param  data      Pointer to the receive buffer.
 * @param  len       Number of bytes to read (must be > 0).
 * @retval I2C_OK    Transaction completed successfully.
 * @retval I2C_ERR   Invalid arguments, or bus/peripheral error.
 * @retval I2C_BUSY  Bus could not be acquired within timeout.
 */
I2C_Status_t i2c_readDevice(uint8_t dev_addr,
                             uint8_t *data, uint32_t len);

#endif /* I2C_DRIVER_H */