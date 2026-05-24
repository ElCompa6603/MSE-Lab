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
 * @file sensor.c
 * @brief Analog sensor module implementation.
 *
 * Wraps the ADC driver to provide a high-level API for reading the
 * potentiometer on PA1 (ADC1 channel 1). This module is responsible
 * for configuring the GPIO pin, since the ADC driver itself is
 * channel-agnostic (it does not assume any specific pin).
 *
 * @author Team 4
 * @date April 29, 2026
 */

/*** Includes ***/
#include "sensor.h"
#include "adc.h"
#include "gpio_driver.h"

/*** Function Definitions ***/

void sensor_init(void)
{
    /* Configure PA1 as analog input via gpio_driver */
    gpio_initPort(GPIO_PORT_A);

    GPIO_PinCfg_t cfg = {
        .mode  = GPIO_MODE_ANALOG,
        .pull  = GPIO_PULL_NONE,
        .otype = GPIO_OTYPE_PUSH_PULL,
        .speed = GPIO_SPEED_LOW
    };
    gpio_setPinMode(GPIO_PORT_A, 1U, &cfg);

    /* Initialize the ADC hardware and power it on */
    adc_init();
    adc_enableAdc();

    /* Select channel 1 (PA1 - potentiometer) */
    adc_setChannel(ADC_CH1);
}

void sensor_startConversion(void)
{
    /* Start continuous conversion: ADC reads repeatedly without stopping */
    adc_startContinuousConversion();
}

uint16_t sensor_readValue(void)
{
    /* Wait for the result and return it (0 to 4095) */
    return adc_readData();
}
