/**
 ******************************************************************************
 * @file    button.c
 * @brief   Button Module Implementation with software debounce
 *          PC13 is active LOW (pressed = LOW) on the Nucleo-64.
 *          The board provides an external pull-up on PC13.
 ******************************************************************************
 */

#include "button.h"

#define DEBOUNCE_SAMPLES    20U

void button_init(void)
{
    GPIO_PinCfg_t cfg;

    gpio_initPort(BUTTON_PORT);         /* FR-2: enable GPIOC clock */

    cfg.mode  = GPIO_MODE_INPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;   /* irrelevant for inputs    */
    cfg.speed = GPIO_SPEED_LOW;         /* irrelevant for inputs    */
    cfg.pull  = GPIO_PULL_NONE;         /* external pull-up on board*/

    gpio_setPinMode(BUTTON_PORT, BUTTON_PIN, &cfg); /* FR-3 */
}

ButtonState_t button_get_state(void)
{
    GPIO_PinState_t raw;
    uint32_t        count = 0U;
    uint32_t        i;

    /* Sample the pin multiple times for debounce */
    for (i = 0U; i < DEBOUNCE_SAMPLES; i++)
    {
        gpio_readPin(BUTTON_PORT, BUTTON_PIN, &raw); /* FR-7 */
        if (raw == GPIO_PIN_LOW)    /* active LOW = pressed */
            count++;

        /* Small inter-sample delay */
        volatile uint32_t d = 100U;
        while (d--) { __asm("nop"); }
    }

    /* Majority vote */
    return (count > (DEBOUNCE_SAMPLES / 2U)) ? BUTTON_PRESSED
                                              : BUTTON_RELEASED;
}
