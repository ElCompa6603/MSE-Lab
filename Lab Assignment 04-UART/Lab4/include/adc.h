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
 * @file adc.h
 * @brief ADC driver for STM32F411RE.
 *
 * ADC_TypeDef, ADC1, and peripheral base addresses are provided by
 * stm32f411xe.h (included via stm32f4xx.h). This header only defines
 * the driver-specific bit masks, enumerations, and function prototypes.
 *
 * @author Team 4
 * @date April 29, 2026
 */

#ifndef __ADC_H__
#define __ADC_H__

/*** Includes ***/
#include <stdint.h>
#include "stm32f4xx.h"   /* Provides ADC_TypeDef, ADC1, RCC, ADC1_BASE */

/*** Preprocessor Definitions ***/

/* ADC1 clock enable: bit 8 of RCC_APB2ENR */
#define ADCEN  (1U << 8)

/* Status Register (SR) flags */
#define SR_EOC   (1U << 1)   /* End of regular conversion; result ready in DR */
#define SR_JEOC  (1U << 2)   /* End of injected conversion                     */

/* Control Register 2 (CR2) bits */
#define CR2_ADON      (1U << 0)   /* ADC power on                              */
#define CR2_CONT      (1U << 1)   /* Continuous conversion mode                */
#define CR2_SWSTART   (1U << 30)  /* Software trigger for regular conversion   */
#define CR2_JSWSTART  (1U << 22)  /* Software trigger for injected conversion  */

/*** Enumerations ***/

/* Driver return status */
typedef enum {
    ADC_OK    = 0,   /* Operation successful */
    ADC_ERROR = 1    /* Invalid parameter    */
} ADC_Status_t;

/* Regular ADC channels (PA0-PA7, PB0-PB1, PC0-PC5, internal channels) */
typedef enum {
    ADC_CH0  = 0,
    ADC_CH1  = 1,
    ADC_CH2  = 2,
    ADC_CH3  = 3,
    ADC_CH4  = 4,
    ADC_CH5  = 5,
    ADC_CH6  = 6,
    ADC_CH7  = 7,
    ADC_CH8  = 8,
    ADC_CH9  = 9,
    ADC_CH10 = 10,
    ADC_CH11 = 11,
    ADC_CH12 = 12,
    ADC_CH13 = 13,
    ADC_CH14 = 14,
    ADC_CH15 = 15,
    ADC_CH16 = 16,
    ADC_CH17 = 17,
    ADC_CH18 = 18
} ADC_Channel_t;

/* Injected ADC channels (1 to 4) */
typedef enum {
    ADC_INJECTED_CH1 = 1,
    ADC_INJECTED_CH2 = 2,
    ADC_INJECTED_CH3 = 3,
    ADC_INJECTED_CH4 = 4
} ADC_InjectedChannel_t;

/*** Function Prototypes ***/

void         adc_init(void);                                                        /* Enable clocks and configure default state             */
void         adc_enableAdc(void);                                                   /* Power on the ADC                                      */
ADC_Status_t adc_setChannel(ADC_Channel_t channel);                                /* Select the regular channel to convert                 */
ADC_Status_t adc_setInjectedChannel(ADC_InjectedChannel_t channel);                /* Select the injected channel to convert                */
void         adc_startSingleConversion(void);                                       /* Trigger a single conversion                           */
void         adc_startContinuousConversion(void);                                   /* Start continuous conversion mode                      */
void         adc_startInjectedConversion(void);                                     /* Trigger injected channel conversion                   */
uint16_t     adc_readData(void);                                                    /* Return the result of the regular conversion           */
uint16_t     adc_readInjectedChannelData(ADC_InjectedChannel_t injectedChannel);   /* Return the result of the specified injected channel   */

#endif /* __ADC_H__ */
