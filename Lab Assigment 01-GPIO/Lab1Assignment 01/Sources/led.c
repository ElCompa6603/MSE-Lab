/**
 ******************************************************************************
 * @file    led.c
 * @brief   LED Module Implementation
 ******************************************************************************
 */

#include "led.h"

void led_init(void)
{
    GPIO_PinCfg_t cfg;

    gpio_initPort(LED_PORT);            /* FR-2: enable GPIOA clock   */

    cfg.mode  = GPIO_MODE_OUTPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    cfg.pull  = GPIO_PULL_NONE;

    gpio_setPinMode(LED_PORT, LED_PIN, &cfg);   /* FR-3 */
    gpio_clearPin(LED_PORT, LED_PIN);           /* FR-5: start with LED off */
}

void led_on(void)
{
    gpio_setPin(LED_PORT, LED_PIN);     /* FR-4 */
}

void led_off(void)
{
    gpio_clearPin(LED_PORT, LED_PIN);   /* FR-5 */
}

void led_toggle(void)
{
    gpio_togglePin(LED_PORT, LED_PIN);  /* FR-6 */
}