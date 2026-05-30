/**
 ******************************************************************************
 * @file    adxl345_driver.h
 * @author  Team 4
 * @brief   ADXL345 Accelerometer Driver — Public API
 *          Interface: I2C (i2c_driver)
 *          STM32F411RE, I2C1, Standard Mode 100 kHz
 *          ADXL345 I2C address: 0x53 (SDO/ALT ADDRESS pin tied LOW)
 ******************************************************************************
 */
#ifndef ADXL345_DRIVER_H
#define ADXL345_DRIVER_H

#include "i2c_driver.h"
#include <stdint.h>

/* ── ADXL345 I2C address ────────────────────────────────────────────────── */
/**
 * @brief 7-bit I2C address of the ADXL345.
 *        SDO/ALT ADDRESS pin LOW  → 0x53
 *        SDO/ALT ADDRESS pin HIGH → 0x1D
 */
#define ADXL345_I2C_ADDR        0x53U

/* ── ADXL345 register map (datasheet Table 19) ──────────────────────────── */
#define ADXL345_REG_DEVID       0x00U   /**< Device ID (read: 0xE5)          */
#define ADXL345_REG_THRESH_TAP  0x1DU   /**< Tap threshold                   */
#define ADXL345_REG_OFSX        0x1EU   /**< X-axis offset                   */
#define ADXL345_REG_OFSY        0x1FU   /**< Y-axis offset                   */
#define ADXL345_REG_OFSZ        0x20U   /**< Z-axis offset                   */
#define ADXL345_REG_BW_RATE     0x2CU   /**< Data rate and power mode ctrl   */
#define ADXL345_REG_POWER_CTL   0x2DU   /**< Power-saving features control   */
#define ADXL345_REG_INT_ENABLE  0x2EU   /**< Interrupt enable control        */
#define ADXL345_REG_INT_MAP     0x2FU   /**< Interrupt mapping control       */
#define ADXL345_REG_INT_SOURCE  0x30U   /**< Source of interrupts            */
#define ADXL345_REG_DATA_FORMAT 0x31U   /**< Data format control             */
#define ADXL345_REG_DATAX0      0x32U   /**< X-axis data LSB                 */
#define ADXL345_REG_DATAX1      0x33U   /**< X-axis data MSB                 */
#define ADXL345_REG_DATAY0      0x34U   /**< Y-axis data LSB                 */
#define ADXL345_REG_DATAY1      0x35U   /**< Y-axis data MSB                 */
#define ADXL345_REG_DATAZ0      0x36U   /**< Z-axis data LSB                 */
#define ADXL345_REG_DATAZ1      0x37U   /**< Z-axis data MSB                 */
#define ADXL345_REG_FIFO_CTL   0x38U   /**< FIFO control                    */
#define ADXL345_REG_FIFO_STATUS 0x39U   /**< FIFO status                     */

/* ── ADXL345 register bit masks ─────────────────────────────────────────── */
#define ADXL345_DEVID_VALUE     0xE5U   /**< Expected device ID              */

/** POWER_CTL register bits */
#define ADXL345_POWER_CTL_MEASURE   (1U << 3)   /**< Enter measurement mode */
#define ADXL345_POWER_CTL_SLEEP     (1U << 2)   /**< Sleep mode             */

/** DATA_FORMAT register bits */
#define ADXL345_DATA_FORMAT_FULL_RES (1U << 3)  /**< Full resolution mode   */
#define ADXL345_DATA_FORMAT_RANGE_2G  0x00U      /**< ±2 g range             */
#define ADXL345_DATA_FORMAT_RANGE_4G  0x01U      /**< ±4 g range             */
#define ADXL345_DATA_FORMAT_RANGE_8G  0x02U      /**< ±8 g range             */
#define ADXL345_DATA_FORMAT_RANGE_16G 0x03U      /**< ±16 g range            */

/** BW_RATE register: output data rate (low-power bit = 0) */
#define ADXL345_BW_RATE_100HZ   0x0AU   /**< 100 Hz output data rate        */
#define ADXL345_BW_RATE_50HZ    0x09U   /**< 50 Hz output data rate         */
#define ADXL345_BW_RATE_25HZ    0x08U   /**< 25 Hz output data rate         */

/* ── Scale factor ───────────────────────────────────────────────────────── */
/**
 * @brief Scale factor in full-resolution mode: 3.9 mg/LSB.
 *        Represented as integer milli-g per LSB × 10 for fixed-point math.
 *        mg = raw * 39 / 10
 */
#define ADXL345_SCALE_MG_PER_LSB_X10   39U

/* ── Return status ──────────────────────────────────────────────────────── */
typedef enum
{
    ADXL345_OK      = 0,    /**< Operation completed successfully            */
    ADXL345_ERR     = 1,    /**< Communication or configuration error        */
    ADXL345_ID_ERR  = 2     /**< Device ID mismatch (wrong device on bus)    */
} ADXL345_Status_t;

/* ── Raw accelerometer data ─────────────────────────────────────────────── */
/**
 * @brief Raw 16-bit signed acceleration values read directly from the sensor.
 *        Units: LSB. Scale: 3.9 mg/LSB in full-resolution mode (±16 g).
 */
typedef struct
{
    int16_t x;  /**< Raw X-axis acceleration (LSB)                          */
    int16_t y;  /**< Raw Y-axis acceleration (LSB)                          */
    int16_t z;  /**< Raw Z-axis acceleration (LSB)                          */
} ADXL345_RawData_t;

/**
 * @brief Scaled acceleration values in milli-g (mg).
 *        1 g = 1000 mg = 9.81 m/s².
 */
typedef struct
{
    int32_t x_mg;   /**< X-axis acceleration in milli-g                     */
    int32_t y_mg;   /**< Y-axis acceleration in milli-g                     */
    int32_t z_mg;   /**< Z-axis acceleration in milli-g                     */
} ADXL345_Data_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initializes the ADXL345 accelerometer over I2C.
 *         Steps performed:
 *           1. Calls i2c_init() to set up the I2C1 peripheral.
 *           2. Reads the DEVID register and verifies it equals 0xE5.
 *           3. Configures BW_RATE  → 100 Hz output data rate.
 *           4. Configures DATA_FORMAT → full-resolution, ±16 g range.
 *           5. Writes POWER_CTL  → enables measurement mode.
 *
 * @retval ADXL345_OK      Sensor found and configured successfully.
 * @retval ADXL345_ID_ERR  DEVID register returned unexpected value.
 * @retval ADXL345_ERR     I2C communication error during initialization.
 */
ADXL345_Status_t adxl345_init(void);

/**
 * @brief  Reads raw X, Y, Z acceleration values from the ADXL345.
 *         Performs a 6-byte burst read starting at DATAX0 (register 0x32).
 *         The sensor outputs data in little-endian format (LSB first).
 *
 * @param  raw  Pointer to an ADXL345_RawData_t struct to fill.
 *              Must not be NULL.
 * @retval ADXL345_OK   Data read successfully.
 * @retval ADXL345_ERR  I2C error or NULL pointer.
 */
ADXL345_Status_t adxl345_readRaw(ADXL345_RawData_t *raw);

/**
 * @brief  Reads acceleration data and converts it to milli-g.
 *         Calls adxl345_readRaw() internally, then applies the scale factor:
 *           mg = raw_LSB × 3.9 mg/LSB  (approximated as raw × 39 / 10)
 *
 * @param  data  Pointer to an ADXL345_Data_t struct to fill.
 *               Must not be NULL.
 * @retval ADXL345_OK   Data read and converted successfully.
 * @retval ADXL345_ERR  I2C error or NULL pointer.
 */
ADXL345_Status_t adxl345_readData(ADXL345_Data_t *data);

/**
 * @brief  Writes a value to an internal ADXL345 register.
 *         Useful for runtime reconfiguration (e.g., changing range or ODR).
 *
 * @param  reg    Target register address (see ADXL345_REG_* defines).
 * @param  value  Byte value to write.
 * @retval ADXL345_OK   Register written successfully.
 * @retval ADXL345_ERR  I2C communication error.
 */
ADXL345_Status_t adxl345_writeReg(uint8_t reg, uint8_t value);

/**
 * @brief  Reads a single byte from an internal ADXL345 register.
 *
 * @param  reg    Source register address (see ADXL345_REG_* defines).
 * @param  value  Pointer to store the read byte. Must not be NULL.
 * @retval ADXL345_OK   Register read successfully.
 * @retval ADXL345_ERR  I2C communication error or NULL pointer.
 */
ADXL345_Status_t adxl345_readReg(uint8_t reg, uint8_t *value);

#endif /* ADXL345_DRIVER_H */
