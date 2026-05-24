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
 * @file pwm.c
 * @brief PWM module implementation using TIM2 channel 1 (PA5).
 *
 * Configuration sequence:
 *   1. Enable clocks for GPIOA and TIM2 via gpio_driver.
 *   2. Configure PA5 as alternate function AF1 (TIM2_CH1) via gpio_driver.
 *   3. Configure TIM2: prescaler, period (ARR), and PWM mode in CCMR1.
 *   4. Enable the output (CCER) and start the counter (CR1).
 *
 * @author Team 4
 * @date April 29, 2026
 */

/*** Includes ***/
#include "pwm.h"
#include "gpio_driver.h"

/*** Function Definitions ***/

/**
 * @brief Initialize TIM2 channel 1 to generate 1 kHz PWM on PA5.
 *
 * PA5 is the LED LD2 pin on the Nucleo-64, so the LED brightness
 * changes directly with the PWM duty cycle.
 *
 * @return None
 */
void PWM_Init(void)
{
    /* Step 1: Enable clocks ------------------------------------------------ */
    gpio_initPort(GPIO_PORT_A);      /* GPIOA clock via gpio_driver */
    RCC->APB1ENR |= TIM2EN;         /* TIM2: bit 0 of APB1ENR      */

    /* Step 2: Configure PA5 as TIM2_CH1 (AF1) via gpio_driver ------------ */
    GPIO_PinCfg_t cfg = {
        .mode  = GPIO_MODE_ALT_FN,
        .pull  = GPIO_PULL_NONE,
        .otype = GPIO_OTYPE_PUSH_PULL,
        .speed = GPIO_SPEED_HIGH
    };
    gpio_setPinMode(GPIO_PORT_A, 5U, &cfg);
    gpio_setAlternateFunction(GPIO_PORT_A, 5U, 1U);  /* AF1 = TIM2_CH1 */

    /* Step 3: Configure TIM2 timer ---------------------------------------- */

    /* Prescaler: divides the APB1 bus clock (16 MHz).
     * PSC = 15 -> counter clock = 16 MHz / (15+1) = 1 MHz */
    TIM2->PSC = PWM_PSC;

    /* Auto-Reload Register: defines the PWM period.
     * ARR = 999 -> counter runs from 0 to 999 -> 1000 counts at 1 MHz = 1 kHz */
    TIM2->ARR = PWM_ARR;

    /* CCMR1: configure channel 1 in PWM mode.
     * OC1M = 110 (PWM mode 1): output high when CNT < CCR1, low when CNT >= CCR1.
     * OC1PE = 1: CCR1 preload is required to avoid glitches when updating. */
    TIM2->CCMR1 = OC1M_PWM1 | OC1PE;

    /* CCER: enable channel 1 output on PA5 */
    TIM2->CCER = CC1E;

    /* CCR1: initial duty cycle = 0% (LED off) */
    TIM2->CCR1 = 0U;

    /* CR1: enable ARR preload and start the counter */
    TIM2->CR1 = CR1_ARPE | CR1_CEN;

    /* EGR: generate an update event to immediately load PSC and ARR */
    TIM2->EGR = UG;
}

/**
 * @brief Update the PWM duty cycle on PA5.
 *
 * Mapping: duty (0-100%) -> CCR1 (0 to PWM_ARR).
 * Formula: CCR1 = (duty * PWM_ARR) / 100
 *
 * Examples:
 *   duty=0   -> CCR1=0   -> LED off
 *   duty=50  -> CCR1=499 -> LED at 50% brightness
 *   duty=100 -> CCR1=999 -> LED at maximum brightness
 *
 * @param duty Duty cycle percentage (0-100).
 * @return None
 */
void PWM_SetDuty(uint32_t duty)
{
    /* Clamp value to 100% to prevent errors */
    if (duty > 100U) { duty = 100U; }

    /* Calculate and write the new compare value */
    TIM2->CCR1 = (duty * PWM_ARR) / 100U;
}
