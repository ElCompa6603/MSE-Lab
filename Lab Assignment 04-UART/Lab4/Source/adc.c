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
 * @file adc.c
 * @brief ADC driver implementation for STM32F411RE.
 *
 * Non-Functional Requirements compliance:
 *   NFR-1 (Lightweight and efficient):
 *     - Direct register access only; no HAL or middleware dependencies.
 *     - No dynamic memory allocation.
 *   NFR-2 (Error handling):
 *     - adc_setChannel() and adc_setInjectedChannel() validate their
 *       channel arguments and return ADC_ERROR on invalid input.
 *   NFR-3 (Real-time performance):
 *     - All functions are O(1); no unbounded waits except in
 *       adc_readData() and adc_readInjectedChannelData(), which poll
 *       the EOC/JEOC hardware flags as required by the ADC peripheral.
 *
 * @author Team 4
 * @date April 29, 2026
 */

/*** Includes ***/
#include "adc.h"
#include "gpio_driver.h"

/*** Function Definitions ***/

/**
 * @brief Initialize the ADC subsystem.
 *
 * FR-1: Enables the ADC1 clock and powers on the peripheral to a known
 * default state. Pin configuration is intentionally left to the caller
 * (e.g. sensor_init) so the driver remains channel-agnostic and portable.
 *
 * Default state after this call:
 *   - ADC1 clock enabled via RCC_APB2ENR.
 *   - Single conversion mode (CONT = 0).
 *   - ADC powered off (ADON = 0); call adc_enableAdc() to activate.
 */
void adc_init(void)
{
    /* FR-1: Enable ADC1 clock */
    RCC->APB2ENR |= ADCEN;

    /* FR-1: Ensure single conversion mode as default state (CONT = 0) */
    ADC1->CR2 &= ~CR2_CONT;
}

/**
 * @brief Enable the ADC instance.
 *
 * FR-2: Sets the ADON bit in CR2 to activate ADC1 for conversions.
 * The ADC requires a stabilization time after power-on before the
 * first conversion is started (handled by hardware internally).
 */
void adc_enableAdc(void)
{
    /* FR-2: Power on the ADC, exit sleep state */
    ADC1->CR2 |= CR2_ADON;
}

/**
 * @brief Configure a regular ADC channel.
 *
 * FR-3: Selects which channel will be converted in the regular sequence.
 * Configures a single-conversion sequence (L = 0 in SQR1).
 *
 * @param channel  Regular channel to select (ADC_CH0 to ADC_CH18).
 * @return ADC_OK on success, ADC_ERROR if channel is out of range.
 */
ADC_Status_t adc_setChannel(ADC_Channel_t channel)
{
    /* FR-3 / NFR-2: Validate channel range */
    if (channel > ADC_CH18) { return ADC_ERROR; }

    /* L=0: regular sequence of a single conversion */
    ADC1->SQR1 = 0U;

    /* SQ1: set the channel to convert */
    ADC1->SQR3 = (uint32_t)channel;

    return ADC_OK;
}

/**
 * @brief Configure an injected ADC channel.
 *
 * FR-4: Selects which injected channel will be converted.
 * Configures a single injected conversion (JL = 0 in JSQR).
 *
 * @param channel  Injected channel to select (ADC_INJECTED_CH1 to ADC_INJECTED_CH4).
 * @return ADC_OK on success, ADC_ERROR if channel is out of range.
 */
ADC_Status_t adc_setInjectedChannel(ADC_InjectedChannel_t channel)
{
    /* FR-4 / NFR-2: Validate injected channel range */
    if (channel < ADC_INJECTED_CH1 || channel > ADC_INJECTED_CH4) { return ADC_ERROR; }

    /* JL=0: injected sequence of a single conversion */
    ADC1->JSQR = 0U;

    /* JSQ4: set the injected channel to convert (bits 19:15) */
    ADC1->JSQR |= (((uint32_t)channel & 0x1FU) << 15);

    return ADC_OK;
}

/**
 * @brief Start a single conversion on the configured regular channel.
 *
 * FR-5: Disables continuous mode and triggers one conversion via
 * software. The EOC flag in SR is set by hardware when done.
 */
void adc_startSingleConversion(void)
{
    /* FR-5: Disable continuous mode to ensure single-shot behavior */
    ADC1->CR2 &= ~CR2_CONT;

    /* FR-5: Trigger the conversion via software start */
    ADC1->CR2 |= CR2_SWSTART;
}

/**
 * @brief Start continuous conversions on the configured regular channel.
 *
 * FR-6: Enables continuous mode and triggers the first conversion.
 * The ADC will keep converting automatically until stopped.
 */
void adc_startContinuousConversion(void)
{
    /* FR-6: Enable continuous mode */
    ADC1->CR2 |= CR2_CONT;

    /* FR-6: Trigger the first conversion; subsequent ones run automatically */
    ADC1->CR2 |= CR2_SWSTART;
}

/**
 * @brief Start a conversion on the configured injected channel.
 *
 * FR-7: Triggers an injected conversion via software (JSWSTART).
 * In a real system this can also be triggered by an external event.
 */
void adc_startInjectedConversion(void)
{
    /* FR-7: Trigger injected channel conversion by software */
    ADC1->CR2 |= CR2_JSWSTART;
}

/**
 * @brief Read the result of the last regular conversion.
 *
 * FR-8: Polls the EOC flag until the hardware signals conversion
 * complete, then returns the 12-bit result from the DR register.
 *
 * @return 12-bit digital value (0–4095) corresponding to the analog input.
 */
uint16_t adc_readData(void)
{
    /* FR-8: Wait until the regular conversion is complete (EOC flag) */
    while (!(ADC1->SR & SR_EOC)) {}

    /* FR-8: Read and return the 12-bit result from the data register */
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/**
 * @brief Read the result of the last injected channel conversion.
 *
 * FR-9: Polls the JEOC flag until the hardware signals conversion
 * complete, then returns the 12-bit result from the appropriate JDR register.
 *
 * @param injectedChannel  Injected channel whose result to read (CH1–CH4).
 * @return 12-bit digital value (0–4095), or 0 for an invalid channel.
 */
uint16_t adc_readInjectedChannelData(ADC_InjectedChannel_t injectedChannel)
{
    /* FR-9: Wait until the injected conversion is complete (JEOC flag) */
    while (!(ADC1->SR & SR_JEOC)) {}

    /* FR-9: Return result from the corresponding injected data register */
    switch (injectedChannel)
    {
        case ADC_INJECTED_CH1: return (uint16_t)(ADC1->JDR1 & 0x0FFFU);
        case ADC_INJECTED_CH2: return (uint16_t)(ADC1->JDR2 & 0x0FFFU);
        case ADC_INJECTED_CH3: return (uint16_t)(ADC1->JDR3 & 0x0FFFU);
        case ADC_INJECTED_CH4: return (uint16_t)(ADC1->JDR4 & 0x0FFFU);
        default:               return 0U;
    }
}
