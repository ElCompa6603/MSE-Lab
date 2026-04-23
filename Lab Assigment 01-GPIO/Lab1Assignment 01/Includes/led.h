/**
 ******************************************************************************
 * @file    led.h
 * @brief   LED Module – Nucleo-64 onboard LED (PA5)
 *          Uses gpio_driver internally (Task 2)
 ******************************************************************************
 */

#ifndef LED_H
#define LED_H

#include "gpio_driver.h"

#define LED_PORT    GPIO_PORT_A
#define LED_PIN     5U

void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);

#endif /* LED_H */