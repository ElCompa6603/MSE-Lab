/**
 ******************************************************************************
 * @file    pwm.h
 * @brief   PWM Module – Generates PWM signal on PA5 for external LED (Task 4)
 *
 * Hardware mapping (NUCLEO-F411RE):
 *   PWM Timer  : TIM2, Channel 1
 *   GPIO Pin   : PA5  ("SCK/D13"  CN5 pin 6)
 *                PA5 → TIM2_CH1 → Alternate Function 1 (AF1)
 *   Circuit    : PA5 → 330 Ω → LED anode → LED cathode → GND (CN6 pin 6)
 *
 * API:
 *   pwm_init(freq_hz)        – Configure timer and GPIO in AF mode
 *   pwm_setSignal(duty_pct)  – Set duty cycle 0–100 %
 *   pwm_start()              – Enable PWM output
 *   pwm_stop()               – Disable PWM output
 ******************************************************************************
 */

#ifndef PWM_H
#define PWM_H

#include "tim_driver.h"
#include "gpio_driver.h"
#include <stdint.h>

/** Timer used for PWM generation */
#define PWM_TIM         TIM_ID_2
#define PWM_CHANNEL     TIM_CH_1

/** GPIO for PWM output: PA5 = TIM2_CH1 = AF1 */
#define PWM_GPIO_PORT   GPIO_PORT_A
#define PWM_GPIO_PIN    5U
#define PWM_GPIO_AF     1U   /* AF1 = TIM2 on PA5 (RM0383 Table 9) */

void pwm_init(uint32_t freq_hz);
void pwm_setSignal(uint8_t duty_percent);
void pwm_start(void);
void pwm_stop(void);

#endif /* PWM_H */
