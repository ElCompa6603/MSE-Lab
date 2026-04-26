/**
 ******************************************************************************
 * @file    led.h
 * @brief   LED Module – External LED for timer blink (Task 3 / Task 5)
 *
 * Hardware:
 *   Pin "PWM/D9"  →  CN9 pin 2  →  PC7
 *   Circuit: PC7 → 330 Ω → LED anode → LED cathode → GND (CN6 pin 6)
 *
 * Task 5 – two simultaneous LEDs:
 *   LED_BLINK (PC7) → 1 Hz blink via timer_delay_ms()  – this module
 *   LED_PWM   (PA5) → 1 kHz PWM  via pwm module        – pwm module
 ******************************************************************************
 */

#ifndef LED_H
#define LED_H

#include "gpio_driver.h"

/* External blink LED – "PWM/D9" CN9 pin 2 */
#define LED_PORT    GPIO_PORT_C
#define LED_PIN     7U

void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);

#endif /* LED_H */
