/**
 ******************************************************************************
 * @file    adxl345_driver.c
 * @author  Team 4
 * @brief   ADXL345 Accelerometer Driver — Implementation
 *          Interface: I2C (i2c_driver)
 *          STM32F411RE, I2C1, Standard Mode 100 kHz
 *          ADXL345 I2C address: 0x53 (SDO/ALT ADDRESS pin tied LOW)
 ******************************************************************************
 */

#include "adxl345_driver.h"

/* ══════════════════════════════════════════════════════════════════════════ */
/*  PRIVATE HELPERS                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Maps an I2C_Status_t result to an ADXL345_Status_t.
 *         Any I2C error (ERR or BUSY) becomes ADXL345_ERR.
 */
static inline ADXL345_Status_t prv_map_status(I2C_Status_t s)
{
    return (s == I2C_OK) ? ADXL345_OK : ADXL345_ERR;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  PUBLIC API IMPLEMENTATION                                                  */
/* ══════════════════════════════════════════════════════════════════════════ */

ADXL345_Status_t adxl345_writeReg(uint8_t reg, uint8_t value)
{
    I2C_Status_t status = i2c_writeRegDevice(ADXL345_I2C_ADDR, reg, &value, 1U);
    return prv_map_status(status);
}

ADXL345_Status_t adxl345_readReg(uint8_t reg, uint8_t *value)
{
    if (value == NULL) return ADXL345_ERR;

    I2C_Status_t status = i2c_readRegDevice(ADXL345_I2C_ADDR, reg, value, 1U);
    return prv_map_status(status);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  INITIALIZATION                                                             */
/* ══════════════════════════════════════════════════════════════════════════ */

ADXL345_Status_t adxl345_init(void)
{
    /* Step 1 — Initialize the I2C1 peripheral */
    i2c_init();

    /* Step 2 — Verify device identity */
    uint8_t dev_id = 0U;
    if (adxl345_readReg(ADXL345_REG_DEVID, &dev_id) != ADXL345_OK)
    {
        return ADXL345_ERR;
    }
    if (dev_id != ADXL345_DEVID_VALUE)
    {
        /* Unexpected device ID — wrong device or wiring issue */
        return ADXL345_ID_ERR;
    }

    /* Step 3 — Set output data rate to 100 Hz (BW_RATE register) */
    if (adxl345_writeReg(ADXL345_REG_BW_RATE, ADXL345_BW_RATE_100HZ) != ADXL345_OK)
    {
        return ADXL345_ERR;
    }

    /* Step 4 — Configure data format: full-resolution, ±16 g range
     *          In full-resolution mode the scale factor is fixed at 3.9 mg/LSB
     *          regardless of the selected range (datasheet p.26). */
    uint8_t fmt = ADXL345_DATA_FORMAT_FULL_RES | ADXL345_DATA_FORMAT_RANGE_16G;
    if (adxl345_writeReg(ADXL345_REG_DATA_FORMAT, fmt) != ADXL345_OK)
    {
        return ADXL345_ERR;
    }

    /* Step 5 — Enable measurement mode (exit standby) */
    if (adxl345_writeReg(ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE) != ADXL345_OK)
    {
        return ADXL345_ERR;
    }

    return ADXL345_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  DATA ACQUISITION                                                           */
/* ══════════════════════════════════════════════════════════════════════════ */

ADXL345_Status_t adxl345_readRaw(ADXL345_RawData_t *raw)
{
    if (raw == NULL) return ADXL345_ERR;

    /*
     * Burst-read 6 bytes starting at DATAX0 (0x32).
     * The ADXL345 auto-increments the register address on each byte when
     * multiple bytes are read in a single I2C transaction (datasheet p.15).
     *
     * Byte layout (little-endian per datasheet):
     *   buf[0] = DATAX0 (X LSB)   buf[1] = DATAX1 (X MSB)
     *   buf[2] = DATAY0 (Y LSB)   buf[3] = DATAY1 (Y MSB)
     *   buf[4] = DATAZ0 (Z LSB)   buf[5] = DATAZ1 (Z MSB)
     */
    uint8_t buf[6] = {0U};
    I2C_Status_t status = i2c_readRegDevice(ADXL345_I2C_ADDR,
                                            ADXL345_REG_DATAX0,
                                            buf, 6U);
    if (status != I2C_OK) return ADXL345_ERR;

    /* Reconstruct signed 16-bit values from little-endian byte pairs */
    raw->x = (int16_t)((uint16_t)buf[1] << 8U | (uint16_t)buf[0]);
    raw->y = (int16_t)((uint16_t)buf[3] << 8U | (uint16_t)buf[2]);
    raw->z = (int16_t)((uint16_t)buf[5] << 8U | (uint16_t)buf[4]);

    return ADXL345_OK;
}

ADXL345_Status_t ladxl345_readData(ADXL345_Data_t *data)
{
    if (data == NULL) return ADXL345_ERR;

    ADXL345_RawData_t raw;
    if (adxl345_readRaw(&raw) != ADXL345_OK) return ADXL345_ERR;

    /*
     * Convert LSB to milli-g using fixed-point arithmetic to avoid floats.
     * Scale factor in full-resolution mode: 3.9 mg/LSB (datasheet Table 1).
     * Approximation: mg = raw × 39 / 10
     * Maximum error: < 0.3% — well within sensor accuracy.
     */
    data->x_mg = ((int32_t)raw.x * (int32_t)ADXL345_SCALE_MG_PER_LSB_X10) / 10;
    data->y_mg = ((int32_t)raw.y * (int32_t)ADXL345_SCALE_MG_PER_LSB_X10) / 10;
    data->z_mg = ((int32_t)raw.z * (int32_t)ADXL345_SCALE_MG_PER_LSB_X10) / 10;

    return ADXL345_OK;
}
