/**
 ******************************************************************************
 * @file    timer.c
 * @brief   Timer Module Implementation (Task 3)
 *
 * Uses TIM4 configured for 1 ms tick via tim_driver.
 * timer_delay_ms() runs the timer N times, waiting for each update event.
 ******************************************************************************
 */

#include "timer.h"

/**
 * @brief Initialize TIM4 for delay generation.
 *        PSC=15999 → tick=1ms, ARR is set per delay call.
 *        The timer is NOT started here; it is started in timer_delay_ms().
 */
void timer_init(void)
{
    tim_initTimer(TIMER_DELAY_TIM);   /* FR-2: enable TIM4 clock */
    /* Pre-configure with 1 ms period; will be overridden per call */
    tim_setTimerMs(TIMER_DELAY_TIM, 1U);  /* FR-3 */
}

/**
 * @brief Block execution for exactly 'ms' milliseconds.
 *
 * Implementation:
 *  - Configure TIM4 ARR = ms-1  (one shot, period = ms ticks of 1 ms each)
 *  - Enable timer  (FR-5)
 *  - Wait for update event (FR-7) → UIF set after ms ms
 *  - Disable timer (FR-6)
 *
 * @param ms  Number of milliseconds to wait (1 … 65535)
 */
void timer_delay_ms(uint32_t ms)
{
    if (ms == 0U) return;

    tim_setTimerMs(TIMER_DELAY_TIM, ms);   /* FR-3: program period */
    tim_enableTimer(TIMER_DELAY_TIM);      /* FR-5: start           */
    tim_waitTimer(TIMER_DELAY_TIM);        /* FR-7: block until end */
    tim_disableTimer(TIMER_DELAY_TIM);     /* FR-6: stop            */
}
