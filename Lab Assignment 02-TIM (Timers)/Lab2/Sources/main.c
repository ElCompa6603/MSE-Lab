/**
 ******************************************************************************
 * @file    main.c
 * @brief   Lab 2 – Application Integration (Task 5)
 *          STM32F411RE / NUCLEO-64
 *
 * ═══════════════════════════════════════════════════════════════════════════
 *  TWO EXTERNAL LEDs – PHYSICAL CONNECTIONS
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  LED1 – PWM dimming
 *    "SCK/D13"  CN5 pin 6  (PA5)
 *    PA5 → 330 Ω → LED1 anode → LED1 cathode → GND (CN6 pin 6)
 *    Signal: PWM 1 kHz, duty 25→50→75→100 % (steps every 4 s)
 *    Scope CH2: 200 µs/div · 2 V/div · trigger rising edge
 *
 *  LED2 – Timer blink
 *    "PWM/D9"  CN9 pin 2  (PC7)
 *    PC7 → 330 Ω → LED2 anode → LED2 cathode → GND (CN6 pin 6)
 *    Signal: square wave 1 Hz (500 ms HIGH, 500 ms LOW)
 *    Scope CH1: 200 ms/div · 2 V/div · trigger rising edge
 *
 *  Common GND: CN6 pin 6 or CN6 pin 7
 *
 * ═══════════════════════════════════════════════════════════════════════════
 *  MODULES USED
 * ═══════════════════════════════════════════════════════════════════════════
 *  led   → PC7 ("PWM/D9")   GPIO output push-pull  – 1 Hz blink
 *  pwm   → PA5 ("SCK/D13")  TIM2_CH1 AF1           – 1 kHz PWM
 *  timer → TIM4              precise 500 ms delay
 *
 *  TIM2 runs freely in hardware (continuous PWM on LED1).
 *  TIM4 is used by timer_delay_ms() to generate the LED2 blink delays.
 *  Both timers operate simultaneously without conflict.
 ******************************************************************************
 */

#include <stdint.h>
#include "gpio_driver.h"
#include "tim_driver.h"
#include "timer.h"
#include "pwm.h"
#include "led.h"

#define PWM_FREQ_HZ     1000U   /* PWM frequency: 1 kHz                     */
#define BLINK_HALF_MS   500U    /* blink half-period: 500 ms                */

/* LED1 duty cycle table – advances every DUTY_HOLD_TICKS half-periods */
static const uint8_t duty_table[] = {25U, 50U, 75U, 100U};
#define DUTY_STEPS       (sizeof(duty_table) / sizeof(duty_table[0]))
#define DUTY_HOLD_TICKS  8U     /* 8 × 500 ms = 4 s per duty step           */

int main(void)
{
    /* 1. Reset all GPIO ports to default state */
    gpio_init();

    /* 2. LED2 – PC7 ("PWM/D9"), GPIO push-pull output for timer blink */
    led_init();

    /* 3. Timer module – TIM4 for precise delay generation */
    timer_init();

    /* 4. LED1 – PA5 ("SCK/D13"), 1 kHz PWM via TIM2_CH1 AF1 */
    pwm_init(PWM_FREQ_HZ);
    pwm_setSignal(duty_table[0]);   /* start at 25 % duty cycle             */
    pwm_start();                    /* TIM2 runs freely, LED1 dims           */

    /* 5. Main loop */
    uint32_t half_tick = 0U;        /* counts 500 ms half-periods            */
    uint8_t  duty_idx  = 0U;        /* current index into duty_table         */

    for (;;)
    {
        /* LED2: toggle every 500 ms → 1 Hz square wave */
        led_toggle();
        timer_delay_ms(BLINK_HALF_MS);  /* precise 500 ms block via TIM4    */
        half_tick++;

        /* LED1: advance duty cycle every 4 s */
        if (half_tick >= DUTY_HOLD_TICKS)
        {
            half_tick = 0U;
            duty_idx  = (uint8_t)((duty_idx + 1U) % DUTY_STEPS);
            pwm_setSignal(duty_table[duty_idx]);
        }
    }
}
