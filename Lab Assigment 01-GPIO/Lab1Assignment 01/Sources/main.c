/**
 ******************************************************************************
 * @file    main.c
 * @brief   Application Integration – Toggle LED on Push-Button press
 * STM32F4xx Nucleo-64
 *
 * Behavior:
 * - gpio_init() resets all ports to their default state (FR-1).
 * - LED (PA5)  initialized as output.
 * - BTN (PC13) initialized as input.
 * - Each RELEASED→PRESSED edge toggles the LED once.
 ******************************************************************************
 */

#include <stdint.h>
#include "gpio_driver.h"
#include "led.h"
#include "button.h"

#define DEBOUNCE 3

int main(void)
{
    uint32_t actual = 0;

    gpio_init();
    led_init();

    for (;;)
    {
        if (BUTTON_PRESSED == button_get_state())
        {
            if (actual++ == DEBOUNCE)
            {
                led_toggle();
            }
        }
        else
        {
            actual = 0;
        }
    }
}