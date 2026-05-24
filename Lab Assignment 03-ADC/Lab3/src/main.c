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
 * @file main.c
 * @brief Main application - ADC Lab for STM32F411RE.
 *
 * Reads the potentiometer on PA1 (ADC1 CH1) and maps the reading
 * to the PWM duty cycle on PA5 (TIM2_CH1, LED LD2).
 *
 * Delay implementation:
 *   Uses TIM3 via tim_driver for the 10 ms loop delay.
 *   TIM2 is reserved for PWM output on PA5 and must not be reused.
 *
 * Observable result:
 *   Turning the potentiometer -> changes the brightness of LED LD2 in real time.
 *
 * Mapping:
 *   ADC = 0    -> duty = 0%   -> LED off
 *   ADC = 2047 -> duty = ~50% -> LED at half brightness
 *   ADC = 4095 -> duty = 100% -> LED at maximum brightness
 *
 * @author Team 4
 * @date April 29, 2026
 */

/*** Includes ***/
#include "gpio_driver.h"  /* GPIO driver (port/pin configuration) */
#include "adc.h"          /* ADC driver                           */
#include "tim_driver.h"   /* TIM driver (delay via TIM3)          */
#include "pwm.h"          /* PWM module (TIM2)                    */
#include "sensor.h"       /* Analog sensor module                 */

/*** Preprocessor Definitions ***/

/* Timer used for the main loop delay (TIM2 is reserved for PWM) */
#define DELAY_TIMER     TIM_ID_3
#define LOOP_DELAY_MS   10U

/*** Function Definitions ***/

/**
 * @brief Application entry point.
 *
 * Initializes all modules and enters the main loop where the ADC
 * controls the PWM duty cycle on each iteration.
 *
 * @return int (never returns in embedded systems)
 */
int main(void)
{
    /* Initialize modules in order:
     *   1. tim_driver : initialize TIM3 for the loop delay.
     *   2. Sensor     : configures the ADC on PA1 (continuous mode).
     *   3. PWM        : configures TIM2 on PA5 (1 kHz). */
    tim_initTimer(DELAY_TIMER);
    sensor_init();
    PWM_Init();

    while (1)
    {
        /* Step 1: Trigger a new sensor conversion */
        sensor_startConversion();

        /* Step 2: Read the 12-bit ADC value (0-4095) */
        uint16_t raw = sensor_readValue();

        /* Step 3: Map ADC reading to duty cycle percentage.
         * Formula: duty = (raw * 100) / 4095
         * Using uint32_t to avoid overflow: 4095 * 100 = 409,500 */
        uint32_t duty = ((uint32_t)raw * 100U) / SENSOR_MAX_COUNT;

        /* Step 4: Update the PWM duty cycle.
         * The change in CCR1 takes effect on the next TIM2 period. */
        PWM_SetDuty(duty);

        /* Step 5: Wait 10 ms before the next reading using tim_driver.
         * tim_setTimerMs configures the period, tim_enableTimer starts
         * the counter, and tim_waitTimer blocks until one period elapses. */
        tim_setTimerMs(DELAY_TIMER, LOOP_DELAY_MS);
        tim_enableTimer(DELAY_TIMER);
        tim_waitTimer(DELAY_TIMER);
        tim_disableTimer(DELAY_TIMER);
    }

    return 0;
}
