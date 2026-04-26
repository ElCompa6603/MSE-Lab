/**
 ******************************************************************************
 * @file    timer.h
 * @brief   Timer Module – Delay generation using TIM driver (Task 3)
 *
 * Uses TIM6... wait, TIM6 is not on F411.
 * Uses TIM4 (APB1, 4 channels) as the delay timer.
 *
 * API:
 *   timer_init()        – Configure TIM4 for 1 ms tick
 *   timer_delay_ms(ms)  – Block for 'ms' milliseconds
 ******************************************************************************
 */

#ifndef TIMER_H
#define TIMER_H

#include "tim_driver.h"
#include <stdint.h>

/** Timer used internally for delay generation */
#define TIMER_DELAY_TIM     TIM_ID_4

void timer_init(void);
void timer_delay_ms(uint32_t ms);

#endif /* TIMER_H */
