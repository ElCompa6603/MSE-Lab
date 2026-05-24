/**
 ******************************************************************************
 * @file    tim_driver.h
 * @brief   TIM Driver - Public API
 *          Compliant with SRS-TIM_Driver v1.1
 *
 * @author Team 4
 *
 * Functional Requirements:
 *   FR-1   tim_init                        – Default state for all timers
 *   FR-2   tim_initTimer                   – Enable clock for a specific timer
 *   FR-3   tim_setTimerMs                  – Configure period in milliseconds
 *   FR-4   tim_setTimerFreq                – Configure frequency in Hz
 *   FR-5   tim_enableTimer                 – Start counting
 *   FR-6   tim_disableTimer                – Stop counting
 *   FR-7   tim_waitTimer                   – Block until update event
 *   FR-8   tim_setTimerCompareChannelValue – Set CCR threshold
 *   FR-9   tim_setTimerCompareMode         – Configure compare mode (PWM etc.)
 *   FR-10  tim_enableTimerCompareChannel   – Enable a CC channel
 *   FR-11  tim_disableTimerCompareChannel  – Disable a CC channel
 *
 * Non-Functional Requirements:
 *   NFR-1  Lightweight  – direct register access, no HAL or middleware
 *   NFR-2  Error handling for invalid timer, channel, or parameter inputs
 *   NFR-3  Real-time safe – no dynamic allocation; O(1) for all functions
 *          except tim_setTimerFreq (O(PSC_MAX) worst-case search) and
 *          tim_waitTimer (polls hardware UIF flag, bounded by timer period)
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

/** System clock frequency in Hz (HSI = 16 MHz; adjust if PLL is enabled). */
#define TIM_SYSCLK_HZ  16000000UL

/* =========================================================================
 * Enumerations
 * ========================================================================= */

/**
 * @brief Supported timer identifiers.
 *
 * Values match the bit positions used in the RCC enable registers
 * only indirectly; the driver uses a switch internally.
 *
 *   TIM1  – Advanced-control,      APB2 (up to 4 CC channels, requires MOE)
 *   TIM2  – General-purpose 32-bit, APB1
 *   TIM3  – General-purpose 16-bit, APB1
 *   TIM4  – General-purpose 16-bit, APB1
 *   TIM5  – General-purpose 32-bit, APB1
 *   TIM9  – General-purpose 16-bit, APB2 (2 CC channels)
 *   TIM10 – General-purpose 16-bit, APB2 (1 CC channel)
 *   TIM11 – General-purpose 16-bit, APB2 (1 CC channel)
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
    TIM_ID_MAX   /* Sentinel — do not use as a timer ID */
} TIM_Id_t;

/** @brief Capture/Compare channel number (1-based, matching STM32 naming). */
typedef enum
{
    TIM_CH_1 = 1,
    TIM_CH_2 = 2,
    TIM_CH_3 = 3,
    TIM_CH_4 = 4
} TIM_Channel_t;

/**
 * @brief Output compare / PWM modes.
 *
 * These values are written directly into the OCxM field of CCMRx.
 * Used by tim_setTimerCompareMode() (FR-9).
 */
typedef enum
{
    TIM_COMPARE_MODE_FROZEN     = 0x0U, /**< Output frozen; no effect on pin        */
    TIM_COMPARE_MODE_ACTIVE     = 0x1U, /**< Pin set active on match                */
    TIM_COMPARE_MODE_INACTIVE   = 0x2U, /**< Pin set inactive on match              */
    TIM_COMPARE_MODE_TOGGLE     = 0x3U, /**< Pin toggles on match                   */
    TIM_COMPARE_MODE_FORCE_LOW  = 0x4U, /**< Pin forced inactive regardless of CNT  */
    TIM_COMPARE_MODE_FORCE_HIGH = 0x5U, /**< Pin forced active regardless of CNT    */
    TIM_COMPARE_MODE_PWM1       = 0x6U, /**< PWM mode 1: active while CNT < CCR     */
    TIM_COMPARE_MODE_PWM2       = 0x7U  /**< PWM mode 2: inactive while CNT < CCR   */
} TIM_CompareMode_t;

/**
 * @brief Driver return codes (NFR-2: error handling).
 *
 * All public functions return one of these values. Negative values
 * indicate errors; TIM_OK (0) indicates success.
 */
typedef enum
{
    TIM_OK                  =  0,  /**< Operation successful                        */
    TIM_ERR_INVALID_TIMER   = -1,  /**< Timer ID out of range                       */
    TIM_ERR_INVALID_CHANNEL = -2,  /**< Channel number out of range                 */
    TIM_ERR_INVALID_PARAM   = -3,  /**< Parameter value invalid (e.g. ms=0, hz=0)   */
    TIM_ERR_CHANNEL_UNSUP   = -4   /**< Channel not available on the selected timer  */
} TIM_Status_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief FR-1: Initialize all TIM peripherals to a default disabled state.
 *
 * Enables clocks for every supported timer, then asserts and releases the
 * peripheral reset to guarantee all registers are at their reset values.
 * After this call no timer is counting and no CC channel is active.
 *
 * NFR-1: Direct RCC register access; no HAL dependency.
 * NFR-3: O(1) — fixed number of register writes.
 */
void tim_init(void);

/**
 * @brief FR-2: Enable the clock for a single timer (selective initialization).
 *
 * Enables the APB clock for the specified timer only, leaving all others
 * unchanged. A read-back flush ensures the peripheral is clocked before
 * the caller accesses its registers.
 *
 * NFR-2: Returns TIM_ERR_INVALID_TIMER for out-of-range IDs.
 * NFR-3: O(1).
 *
 * @param timer  Timer to initialize (TIM_ID_1 to TIM_ID_11).
 * @return TIM_OK, or TIM_ERR_INVALID_TIMER.
 */
TIM_Status_t tim_initTimer(TIM_Id_t timer);

/**
 * @brief FR-3: Configure a timer period in milliseconds.
 *
 * Computes PSC and ARR so that the timer generates one update event
 * every 'ms' milliseconds at TIM_SYSCLK_HZ:
 *   PSC = (SYSCLK / 1000) - 1  → tick = 1 ms
 *   ARR = ms - 1
 *
 * Stops the timer, writes PSC/ARR, then forces an update event (UG)
 * so the shadow registers are loaded immediately.
 *
 * Constraint: 16-bit timers (all except TIM2/TIM5) accept ms up to 65535.
 *
 * NFR-2: Returns TIM_ERR_INVALID_PARAM for ms=0 or out-of-range values.
 * NFR-3: O(1).
 *
 * @param timer  Target timer.
 * @param ms     Desired period in milliseconds (must be > 0).
 * @return TIM_OK, TIM_ERR_INVALID_TIMER, or TIM_ERR_INVALID_PARAM.
 */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms);

/**
 * @brief FR-4: Configure a timer to generate update events at a given frequency.
 *
 * Searches for the smallest PSC such that ARR fits within the timer's
 * register width, then writes both registers:
 *   ARR = (SYSCLK / ((PSC+1) * hz)) - 1
 *
 * NFR-2: Returns TIM_ERR_INVALID_PARAM for hz=0 or unachievable frequencies.
 * NFR-3: O(PSC_MAX) worst-case search; bounded by the 16-bit PSC register.
 *
 * @param timer  Target timer.
 * @param hz     Desired update frequency in Hz (must be > 0).
 * @return TIM_OK, TIM_ERR_INVALID_TIMER, or TIM_ERR_INVALID_PARAM.
 */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz);

/**
 * @brief FR-5: Enable (start) a timer by setting the CEN bit in CR1.
 *
 * The timer begins counting from its current CNT value. Call
 * tim_setTimerMs() or tim_setTimerFreq() before enabling to ensure
 * PSC and ARR are configured.
 *
 * NFR-2: Returns TIM_ERR_INVALID_TIMER for out-of-range IDs.
 * NFR-3: O(1).
 *
 * @param timer  Timer to enable.
 * @return TIM_OK, or TIM_ERR_INVALID_TIMER.
 */
TIM_Status_t tim_enableTimer(TIM_Id_t timer);

/**
 * @brief FR-6: Disable (stop) a timer by clearing the CEN bit in CR1.
 *
 * The counter freezes at its current value. The timer can be restarted
 * with tim_enableTimer() without reconfiguring PSC/ARR.
 *
 * NFR-2: Returns TIM_ERR_INVALID_TIMER for out-of-range IDs.
 * NFR-3: O(1).
 *
 * @param timer  Timer to disable.
 * @return TIM_OK, or TIM_ERR_INVALID_TIMER.
 */
TIM_Status_t tim_disableTimer(TIM_Id_t timer);

/**
 * @brief FR-7: Block execution until the timer generates an update event.
 *
 * Polls the UIF flag in SR. The timer must already be running (CEN=1);
 * calling this on a stopped timer will block indefinitely.
 * Clears UIF after detection so successive calls work correctly.
 *
 * NFR-2: Returns TIM_ERR_INVALID_TIMER for out-of-range IDs.
 * NFR-3: Blocking duration is bounded by the configured timer period.
 *
 * @param timer  Running timer to wait on.
 * @return TIM_OK, or TIM_ERR_INVALID_TIMER.
 */
TIM_Status_t tim_waitTimer(TIM_Id_t timer);

/**
 * @brief FR-8: Write the compare threshold value for a CC channel (CCRx).
 *
 * The timer generates a compare event when CNT reaches this value.
 * In PWM mode the duty cycle is proportional to CCRx / ARR.
 *
 * NFR-2: Returns TIM_ERR_INVALID_TIMER, TIM_ERR_INVALID_CHANNEL, or
 *        TIM_ERR_CHANNEL_UNSUP for invalid inputs.
 * NFR-3: O(1).
 *
 * @param timer    Target timer.
 * @param channel  CC channel (TIM_CH_1 to TIM_CH_4).
 * @param value    Compare threshold value to write into CCRx.
 * @return TIM_OK, or an error code.
 */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value);

/**
 * @brief FR-9: Configure the output compare / PWM mode for a CC channel.
 *
 * Writes the OCxM field in CCMRx and enables output preload (OCxPE)
 * for stable PWM operation. Also sets ARPE in CR1.
 *
 * NFR-2: Returns error codes for invalid timer or channel.
 * NFR-3: O(1).
 *
 * @param timer    Target timer.
 * @param channel  CC channel to configure.
 * @param mode     Compare/PWM mode from TIM_CompareMode_t.
 * @return TIM_OK, or an error code.
 */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t          timer,
                                     TIM_Channel_t     channel,
                                     TIM_CompareMode_t mode);

/**
 * @brief FR-10: Enable a CC channel output by setting CCxE in CCER.
 *
 * For TIM1 (advanced-control timer), also sets the MOE bit in BDTR,
 * which is required before any CC output can drive a pin.
 *
 * NFR-2: Returns error codes for invalid timer or channel.
 * NFR-3: O(1).
 *
 * @param timer    Target timer.
 * @param channel  CC channel to enable.
 * @return TIM_OK, or an error code.
 */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel);

/**
 * @brief FR-11: Disable a CC channel output by clearing CCxE in CCER.
 *
 * Prevents the channel from driving its output pin. The CCRx value
 * is preserved and the channel can be re-enabled without reconfiguration.
 *
 * NFR-2: Returns error codes for invalid timer or channel.
 * NFR-3: O(1).
 *
 * @param timer    Target timer.
 * @param channel  CC channel to disable.
 * @return TIM_OK, or an error code.
 */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel);

#endif /* TIM_DRIVER_H */
