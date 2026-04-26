/**
 ******************************************************************************
 * @file    tim_driver.h
 * @brief   TIM Driver – Public API
 *          Compliant with SRS-TIM_Driver v1.1
 *
 * Functional Requirements:
 *   FR-1   tim_init                       – Default state for all timers
 *   FR-2   tim_initTimer                  – Enable clock for a specific timer
 *   FR-3   tim_setTimerMs                 – Configure period in milliseconds
 *   FR-4   tim_setTimerFreq               – Configure frequency in Hz
 *   FR-5   tim_enableTimer                – Start counting
 *   FR-6   tim_disableTimer               – Stop counting
 *   FR-7   tim_waitTimer                  – Block until update event
 *   FR-8   tim_setTimerCompareChannelValue– Set CCR threshold
 *   FR-9   tim_setTimerCompareMode        – Configure compare mode (PWM etc.)
 *   FR-10  tim_enableTimerCompareChannel  – Enable a CC channel
 *   FR-11  tim_disableTimerCompareChannel – Disable a CC channel
 *
 * Non-Functional Requirements:
 *   NFR-1  Lightweight – direct register access, no HAL dependency
 *   NFR-2  Error handling for invalid timer or channel inputs
 *   NFR-3  Real-time safe – no dynamic allocation
 *
 * Target: STM32F411RE (RM0383)
 * System clock assumed: 16 MHz (HSI, no PLL)
 ******************************************************************************
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/* =========================================================================
 * Constants
 * ========================================================================= */

/** System clock in Hz (HSI = 16 MHz, adjust if PLL is used) */
#define TIM_SYSCLK_HZ   16000000UL

/* =========================================================================
 * Enumerations
 * ========================================================================= */

/**
 * @brief  Supported timer identifiers.
 *         TIM1  – Advanced, APB2
 *         TIM2  – General purpose 32-bit, APB1
 *         TIM3  – General purpose 16-bit, APB1
 *         TIM4  – General purpose 16-bit, APB1
 *         TIM5  – General purpose 32-bit, APB1
 *         TIM9  – General purpose 16-bit, APB2
 *         TIM10 – General purpose 16-bit, APB2
 *         TIM11 – General purpose 16-bit, APB2
 */
typedef enum
{
    TIM_ID_1  = 0,
    TIM_ID_2,
    TIM_ID_3,
    TIM_ID_4,
    TIM_ID_5,
    TIM_ID_9,
    TIM_ID_10,
    TIM_ID_11,
    TIM_ID_MAX
} TIM_Id_t;

/** @brief Capture/Compare channel number (1-based, as per STM32 naming) */
typedef enum
{
    TIM_CH_1 = 1,
    TIM_CH_2 = 2,
    TIM_CH_3 = 3,
    TIM_CH_4 = 4
} TIM_Channel_t;

/** @brief Compare / PWM modes written directly into CCMRx.OCxM bits */
typedef enum
{
    TIM_COMPARE_MODE_FROZEN        = 0x0U, /**< Output frozen, no effect   */
    TIM_COMPARE_MODE_ACTIVE        = 0x1U, /**< Active on match            */
    TIM_COMPARE_MODE_INACTIVE      = 0x2U, /**< Inactive on match          */
    TIM_COMPARE_MODE_TOGGLE        = 0x3U, /**< Toggle on match            */
    TIM_COMPARE_MODE_FORCE_LOW     = 0x4U, /**< Force inactive level       */
    TIM_COMPARE_MODE_FORCE_HIGH    = 0x5U, /**< Force active level         */
    TIM_COMPARE_MODE_PWM1          = 0x6U, /**< PWM mode 1 (active while CNT<CCR) */
    TIM_COMPARE_MODE_PWM2          = 0x7U  /**< PWM mode 2 (inactive while CNT<CCR) */
} TIM_CompareMode_t;

/** @brief Return codes */
typedef enum
{
    TIM_OK                   =  0,
    TIM_ERR_INVALID_TIMER    = -1,
    TIM_ERR_INVALID_CHANNEL  = -2,
    TIM_ERR_INVALID_PARAM    = -3,
    TIM_ERR_CHANNEL_UNSUP    = -4  /**< Channel not available on this timer */
} TIM_Status_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/* FR-1: Subsystem initialization */
void         tim_init(void);

/* FR-2: Individual timer initialization (clock enable) */
TIM_Status_t tim_initTimer(TIM_Id_t timer);

/* FR-3: Set period in milliseconds */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms);

/* FR-4: Set frequency in Hz */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz);

/* FR-5: Enable (start) a timer */
TIM_Status_t tim_enableTimer(TIM_Id_t timer);

/* FR-6: Disable (stop) a timer */
TIM_Status_t tim_disableTimer(TIM_Id_t timer);

/* FR-7: Block until update event (one period elapsed) */
TIM_Status_t tim_waitTimer(TIM_Id_t timer);

/* FR-8: Set compare channel threshold value */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value);

/* FR-9: Set compare/PWM mode for a channel */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t         timer,
                                     TIM_Channel_t    channel,
                                     TIM_CompareMode_t mode);

/* FR-10: Enable a capture/compare channel output */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel);

/* FR-11: Disable a capture/compare channel output */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel);

#endif /* TIM_DRIVER_H */
