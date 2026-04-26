/**
 ******************************************************************************
 * @file    pwm.c
 * @brief   PWM Module Implementation (Task 4)
 *
 * Uses TIM2_CH1 on PA5 (AF1) to generate a PWM signal.
 *
 * Duty cycle formula:
 *   CCR1 = (duty_percent * (ARR + 1)) / 100
 *
 * PWM mode 1 (TIM_COMPARE_MODE_PWM1):
 *   Output HIGH while CNT < CCR1, LOW while CNT >= CCR1.
 ******************************************************************************
 */

#include "pwm.h"

/* Cached ARR value so pwm_setSignal can compute CCR correctly */
static uint32_t s_arr = 0U;

/**
 * @brief  Initialize PWM on TIM2_CH1 / PA5.
 *
 * Steps:
 *  1. Enable GPIO clock, configure PA5 as Alternate Function (AF1).
 *  2. Initialize TIM2, set frequency.
 *  3. Configure CH1 in PWM Mode 1.
 *  4. Set 0 % duty cycle (CCR1 = 0) initially.
 *
 * @param freq_hz  PWM frequency in Hz (e.g. 1000 for 1 kHz)
 */
void pwm_init(uint32_t freq_hz)
{
    /* --- GPIO configuration: PA5 as TIM2_CH1 (AF1) --- */
    gpio_initPort(PWM_GPIO_PORT);

    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ALT_FN;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_HIGH;
    cfg.pull  = GPIO_PULL_NONE;
    gpio_setPinMode(PWM_GPIO_PORT, PWM_GPIO_PIN, &cfg);
    gpio_setAlternateFunction(PWM_GPIO_PORT, PWM_GPIO_PIN, PWM_GPIO_AF); /* FR-8 GPIO */

    /* --- TIM2 configuration --- */
    tim_initTimer(PWM_TIM);                                /* FR-2 TIM */
    tim_setTimerFreq(PWM_TIM, freq_hz);                   /* FR-4 TIM */

    /* Cache ARR so duty cycle can be computed later */
    s_arr = TIM2->ARR;

    /* Configure CH1 in PWM Mode 1 */
    tim_setTimerCompareMode(PWM_TIM, PWM_CHANNEL,
                            TIM_COMPARE_MODE_PWM1);        /* FR-9 TIM */

    /* Start with 0 % duty cycle */
    tim_setTimerCompareChannelValue(PWM_TIM, PWM_CHANNEL, 0U); /* FR-8 TIM */
}

/**
 * @brief  Set PWM duty cycle.
 * @param  duty_percent  0 = always OFF, 100 = always ON.
 */
void pwm_setSignal(uint8_t duty_percent)
{
    if (duty_percent > 100U) duty_percent = 100U;

    uint32_t ccr = (duty_percent * (s_arr + 1UL)) / 100UL;
    tim_setTimerCompareChannelValue(PWM_TIM, PWM_CHANNEL, ccr); /* FR-8 TIM */
}

/**
 * @brief  Start PWM output (enable channel + start timer).
 */
void pwm_start(void)
{
    tim_enableTimerCompareChannel(PWM_TIM, PWM_CHANNEL);  /* FR-10 TIM */
    tim_enableTimer(PWM_TIM);                             /* FR-5 TIM  */
}

/**
 * @brief  Stop PWM output (disable channel + stop timer).
 */
void pwm_stop(void)
{
    tim_disableTimerCompareChannel(PWM_TIM, PWM_CHANNEL); /* FR-11 TIM */
    tim_disableTimer(PWM_TIM);                            /* FR-6 TIM  */
}
