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
 * @file sensor.h
 * @brief Analog sensor module for STM32F411RE.
 *
 * Wraps the ADC driver to provide a high-level API oriented toward
 * reading analog sensors (potentiometer, temperature, light...).
 *
 * Typical usage:
 *   sensor_init();
 *   sensor_startConversion();
 *   uint16_t val = sensor_readValue();
 *
 * @author Team 4
 * @date April 29, 2026
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Reference voltage in mV used to calculate the sensor voltage */
#define SENSOR_VREF_MV   3300U   /* 3.3 V = 3300 mV  */
#define SENSOR_MAX_COUNT 4095U   /* 12-bit resolution */

/*** Function Prototypes ***/

/**
 * @brief Initialize the sensor module (calls ADC_Init internally).
 * @return None
 */
void sensor_init(void);

/**
 * @brief Trigger a new ADC conversion by software.
 *
 * In continuous mode the ADC is already converting; this function
 * allows restarting the cycle if needed.
 *
 * @return None
 */
void sensor_startConversion(void);

/**
 * @brief Wait for end of conversion and return the 12-bit value.
 * @return ADC count from 0 (0V) to 4095 (3.3V).
 */
uint16_t sensor_readValue(void);

#endif /* __SENSOR_H__ */
