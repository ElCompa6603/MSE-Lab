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
 * @file pwm.h
 * @brief PWM module using TIM2 channel 1 (PA5) for STM32F411RE.
 *
 * TIM_TypeDef, TIM2, and peripheral base addresses are provided by
 * stm32f411xe.h (included via stm32f4xx.h). This header only defines
 * the driver-specific bit masks, timer parameters, and function prototypes.
 *
 * Connection:
 *   PA5 -> TIM2_CH1 (AF1) -> Nucleo LED LD2
 *
 * PWM frequency:
 *   APB1 clock = 16 MHz
 *   Prescaler  = 15  -> TIM2 clock = 16 MHz / 16 = 1 MHz
 *   ARR        = 999 -> frequency  = 1 MHz / 1000 = 1 kHz
 *
 * Duty cycle:
 *   CCR1 = 0   ->   0% (LED off)
 *   CCR1 = 500 ->  50% (half brightness)
 *   CCR1 = 999 -> 100% (LED at maximum)
 *
 * @author Team 4
 * @date April 29, 2026
 */

#ifndef __PWM_H__
#define __PWM_H__

/*** Includes ***/
#include <stdint.h>
#include "stm32f4xx.h"   /* Provides TIM_TypeDef, TIM2, RCC */

/*** Preprocessor Definitions ***/

/* RCC_APB1ENR bit to enable TIM2 clock */
#define TIM2EN  (1U << 0)

/* CR1 register bits */
#define CR1_ARPE  (1U << 7)  /* Auto-Reload Preload: ARR applied on next overflow */
#define CR1_CEN   (1U << 0)  /* Counter Enable: starts the counter                */

/* CCMR1 register bits (channel 1 output configuration)
 * OC1M[2:0] (bits [6:4]): output mode
 *   110 = PWM mode 1: output active (high) when CNT < CCR1 */
#define OC1M_PWM1  (6U << 4)  /* PWM mode 1                                        */
#define OC1PE      (1U << 3)  /* Output Compare 1 Preload Enable (required in PWM) */

/* CCER register bits */
#define CC1E  (1U << 0)  /* Capture/Compare 1 Output Enable: enables channel 1 output */

/* EGR register bits */
#define UG  (1U << 0)  /* Update Generation: immediately loads PSC and ARR */

/* Timer parameters */
#define PWM_PSC  15U   /* Prescaler: 16 MHz / 16 = 1 MHz counter clock */
#define PWM_ARR  999U  /* Auto-Reload: 1000 counts -> 1 kHz             */

/*** Function Prototypes ***/

/**
 * @brief Initialize TIM2 to generate 1 kHz PWM on PA5 (LD2).
 * @return None
 */
void PWM_Init(void);

/**
 * @brief Update the PWM duty cycle.
 * @param duty Duty cycle from 0 to 100 (percentage).
 *             0   = always low  (LED off).
 *             100 = always high (LED at maximum).
 * @return None
 */
void PWM_SetDuty(uint32_t duty);

#endif /* __PWM_H__ */
