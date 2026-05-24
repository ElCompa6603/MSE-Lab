/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   TIM Driver Implementation
 *          Compliant with SRS-TIM_Driver v1.1
 *
 * @author Team 4
 *
 * Non-Functional Requirements compliance:
 *   NFR-1 (Lightweight and efficient):
 *     - Direct register access only; no HAL or middleware dependencies.
 *     - No dynamic memory allocation anywhere in the driver.
 *   NFR-2 (Error handling):
 *     - Every public function validates its timer ID before touching any
 *       register. Channel functions additionally validate the channel number
 *       and its availability on the selected timer.
 *   NFR-3 (Real-time performance):
 *     - All functions are O(1) except:
 *         tim_setTimerFreq: O(PSC_MAX) worst-case PSC search, bounded by
 *                           the 16-bit PSC register (max 65535 iterations).
 *         tim_waitTimer:    polls hardware UIF; duration bounded by the
 *                           configured timer period.
 *
 * Design notes:
 *   - PSC and ARR are computed from TIM_SYSCLK_HZ (16 MHz HSI).
 *   - tim_setTimerMs:  PSC = 15999 → tick = 1 ms; ARR = ms - 1.
 *   - tim_setTimerFreq: iterates PSC from 0 upward until ARR fits in register.
 *   - TIM1 requires the MOE bit in BDTR for CC output; handled in FR-10.
 *   - TIM9 supports only 2 CC channels; TIM10/TIM11 support only 1.
 *
 * Target: STM32F411RE (RM0383)
 ******************************************************************************
 */

#include "tim_driver.h"

/* =========================================================================
 * Private helper declarations
 * ========================================================================= */

static TIM_TypeDef  *prv_getTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch);
static uint8_t       prv_maxChannels(TIM_Id_t timer);

/* =========================================================================
 * FR-1 – tim_init
 * Initialize all supported TIM peripherals to a known disabled state.
 * ========================================================================= */
void tim_init(void)
{
    /* FR-1 / NFR-1: Enable clocks for every supported timer in one write */
    RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN  |
                     RCC_APB2ENR_TIM9EN  |
                     RCC_APB2ENR_TIM10EN |
                     RCC_APB2ENR_TIM11EN);

    RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN |
                     RCC_APB1ENR_TIM3EN |
                     RCC_APB1ENR_TIM4EN |
                     RCC_APB1ENR_TIM5EN);

    /* FR-1: Assert peripheral reset to guarantee all registers are cleared */
    RCC->APB2RSTR |=  (RCC_APB2RSTR_TIM1RST  |
                       RCC_APB2RSTR_TIM9RST  |
                       RCC_APB2RSTR_TIM10RST |
                       RCC_APB2RSTR_TIM11RST);
    RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM1RST  |
                       RCC_APB2RSTR_TIM9RST  |
                       RCC_APB2RSTR_TIM10RST |
                       RCC_APB2RSTR_TIM11RST);

    RCC->APB1RSTR |=  (RCC_APB1RSTR_TIM2RST |
                       RCC_APB1RSTR_TIM3RST |
                       RCC_APB1RSTR_TIM4RST |
                       RCC_APB1RSTR_TIM5RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST |
                       RCC_APB1RSTR_TIM3RST |
                       RCC_APB1RSTR_TIM4RST |
                       RCC_APB1RSTR_TIM5RST);
}

/* =========================================================================
 * FR-2 – tim_initTimer
 * Enable the APB clock for a single timer (selective initialization).
 * ========================================================================= */
TIM_Status_t tim_initTimer(TIM_Id_t timer)
{
    /* NFR-2: Validate timer ID before accessing any register */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }

    /* FR-2: Enable the clock for the requested timer only */
    switch (timer)
    {
        case TIM_ID_1:  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;  break;
        case TIM_ID_2:  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  break;
        case TIM_ID_3:  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  break;
        case TIM_ID_4:  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  break;
        case TIM_ID_5:  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  break;
        case TIM_ID_9:  RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;  break;
        case TIM_ID_10: RCC->APB2ENR |= RCC_APB2ENR_TIM10EN; break;
        case TIM_ID_11: RCC->APB2ENR |= RCC_APB2ENR_TIM11EN; break;
        default: break;
    }

    /* Read-back flush: ensures the peripheral is clocked before register access */
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;

    return TIM_OK;
}

/* =========================================================================
 * FR-3 – tim_setTimerMs
 * Configure a timer period in milliseconds.
 *
 * Strategy:
 *   PSC = (SYSCLK / 1000) - 1  →  each counter tick = 1 ms (PSC = 15999)
 *   ARR = ms - 1
 *   Constraint: 16-bit timers (all except TIM2/TIM5) require ARR <= 0xFFFF.
 * ========================================================================= */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms)
{
    /* NFR-2: Validate timer and parameter */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }
    if (ms == 0U)     { return TIM_ERR_INVALID_PARAM; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    uint32_t psc = (TIM_SYSCLK_HZ / 1000UL) - 1UL;  /* = 15999 at 16 MHz */
    uint32_t arr = ms - 1UL;

    /* NFR-2: 16-bit timers cannot hold values larger than 0xFFFF */
    if (timer != TIM_ID_2 && timer != TIM_ID_5)
    {
        if (arr > 0xFFFFUL || psc > 0xFFFFUL) { return TIM_ERR_INVALID_PARAM; }
    }

    /* FR-3: Stop timer, write PSC/ARR, force shadow register update */
    TIMx->CR1 &= ~TIM_CR1_CEN;   /* Stop counter before reconfiguring    */
    TIMx->PSC  = psc;
    TIMx->ARR  = arr;
    TIMx->CNT  = 0U;
    TIMx->SR  &= ~TIM_SR_UIF;    /* Clear any stale update flag           */
    TIMx->EGR |= TIM_EGR_UG;     /* Force PSC/ARR into shadow registers   */
    TIMx->SR  &= ~TIM_SR_UIF;    /* Clear UIF raised by the UG event      */

    return TIM_OK;
}

/* =========================================================================
 * FR-4 – tim_setTimerFreq
 * Configure a timer to generate update events at a given frequency (Hz).
 *
 * Strategy:
 *   Iterate PSC from 0 upward; for each PSC compute:
 *     ARR = (SYSCLK / ((PSC+1) * hz)) - 1
 *   Stop at the first PSC where ARR fits within the timer's register width.
 *   This minimises PSC (maximises counter resolution) for the requested Hz.
 * ========================================================================= */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz)
{
    /* NFR-2: Validate timer and parameter */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }
    if (hz == 0U)     { return TIM_ERR_INVALID_PARAM; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* TIM2 and TIM5 have 32-bit ARR; all others are 16-bit */
    uint8_t  is32bit = (timer == TIM_ID_2 || timer == TIM_ID_5);
    uint32_t arr_max = is32bit ? 0xFFFFFFFFUL : 0xFFFFUL;

    uint32_t psc   = 0U;
    uint32_t arr   = 0U;
    uint8_t  found = 0U;

    /* FR-4: Search for the smallest PSC that yields a valid ARR */
    for (psc = 0U; psc <= 0xFFFFUL; psc++)
    {
        uint32_t div = (psc + 1UL) * hz;
        if (div == 0U) { continue; }  /* Guard against overflow edge case */

        arr = (TIM_SYSCLK_HZ / div);
        if (arr > 0U) { arr -= 1UL; }

        if (arr <= arr_max)
        {
            found = 1U;
            break;
        }
    }

    /* NFR-2: No valid (PSC, ARR) pair found for the requested frequency */
    if (found == 0U) { return TIM_ERR_INVALID_PARAM; }

    /* FR-4: Stop timer, write PSC/ARR, force shadow register update */
    TIMx->CR1 &= ~TIM_CR1_CEN;
    TIMx->PSC  = psc;
    TIMx->ARR  = arr;
    TIMx->CNT  = 0U;
    TIMx->SR  &= ~TIM_SR_UIF;
    TIMx->EGR |= TIM_EGR_UG;
    TIMx->SR  &= ~TIM_SR_UIF;

    return TIM_OK;
}

/* =========================================================================
 * FR-5 – tim_enableTimer
 * Start the timer counter by setting the CEN bit in CR1.
 * ========================================================================= */
TIM_Status_t tim_enableTimer(TIM_Id_t timer)
{
    /* NFR-2: Validate timer ID */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }

    /* FR-5: Set CEN to begin counting and generating update events */
    prv_getTimer(timer)->CR1 |= TIM_CR1_CEN;

    return TIM_OK;
}

/* =========================================================================
 * FR-6 – tim_disableTimer
 * Stop the timer counter by clearing the CEN bit in CR1.
 * ========================================================================= */
TIM_Status_t tim_disableTimer(TIM_Id_t timer)
{
    /* NFR-2: Validate timer ID */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }

    /* FR-6: Clear CEN to stop counting and event generation */
    prv_getTimer(timer)->CR1 &= ~TIM_CR1_CEN;

    return TIM_OK;
}

/* =========================================================================
 * FR-7 – tim_waitTimer
 * Block until the timer generates one update event (UIF flag set).
 * The timer must already be running (CEN = 1) before calling this.
 * ========================================================================= */
TIM_Status_t tim_waitTimer(TIM_Id_t timer)
{
    /* NFR-2: Validate timer ID */
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* FR-7: Poll UIF until the counter reaches ARR (one period elapsed) */
    while (!(TIMx->SR & TIM_SR_UIF)) { /* spin — bounded by timer period */ }

    /* Clear UIF so the next call to tim_waitTimer() works correctly */
    TIMx->SR &= ~TIM_SR_UIF;

    return TIM_OK;
}

/* =========================================================================
 * FR-8 – tim_setTimerCompareChannelValue
 * Write the compare threshold into the CCRx register for a given channel.
 * ========================================================================= */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value)
{
    /* NFR-2: Validate timer and channel */
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) { return st; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* FR-8: Write value into the appropriate CCR register */
    switch (channel)
    {
        case TIM_CH_1: TIMx->CCR1 = value; break;
        case TIM_CH_2: TIMx->CCR2 = value; break;
        case TIM_CH_3: TIMx->CCR3 = value; break;
        case TIM_CH_4: TIMx->CCR4 = value; break;
        default: return TIM_ERR_INVALID_CHANNEL;
    }

    return TIM_OK;
}

/* =========================================================================
 * FR-9 – tim_setTimerCompareMode
 * Configure the OCxM field in CCMRx and enable output preload (OCxPE).
 * Also enables auto-reload preload (ARPE) in CR1 for stable PWM.
 * ========================================================================= */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t          timer,
                                     TIM_Channel_t     channel,
                                     TIM_CompareMode_t mode)
{
    /* NFR-2: Validate timer and channel */
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) { return st; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);
    uint32_t     ocm  = (uint32_t)mode & 0x7UL;  /* OCxM is 3 bits */

    /* FR-9: Write OCxM and set OCxPE (output preload enable) */
    switch (channel)
    {
        case TIM_CH_1:
            /* CCMR1 bits [6:4] = OC1M, bit 3 = OC1PE, bits [1:0] = CC1S (clear for output) */
            TIMx->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_CC1S);
            TIMx->CCMR1 |=  (ocm << 4U) | TIM_CCMR1_OC1PE;
            break;
        case TIM_CH_2:
            /* CCMR1 bits [14:12] = OC2M, bit 11 = OC2PE, bits [9:8] = CC2S */
            TIMx->CCMR1 &= ~(TIM_CCMR1_OC2M | TIM_CCMR1_CC2S);
            TIMx->CCMR1 |=  (ocm << 12U) | TIM_CCMR1_OC2PE;
            break;
        case TIM_CH_3:
            TIMx->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_CC3S);
            TIMx->CCMR2 |=  (ocm << 4U) | TIM_CCMR2_OC3PE;
            break;
        case TIM_CH_4:
            TIMx->CCMR2 &= ~(TIM_CCMR2_OC4M | TIM_CCMR2_CC4S);
            TIMx->CCMR2 |=  (ocm << 12U) | TIM_CCMR2_OC4PE;
            break;
        default: return TIM_ERR_INVALID_CHANNEL;
    }

    /* Enable auto-reload preload so ARR updates take effect at next overflow */
    TIMx->CR1 |= TIM_CR1_ARPE;

    return TIM_OK;
}

/* =========================================================================
 * FR-10 – tim_enableTimerCompareChannel
 * Set the CCxE bit in CCER to activate the channel output pin.
 * For TIM1 (advanced-control), also set MOE in BDTR.
 * ========================================================================= */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel)
{
    /* NFR-2: Validate timer and channel */
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) { return st; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* FR-10: Each channel occupies 4 bits in CCER; CCxE is bit 0 of its nibble */
    uint32_t shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER |= (1UL << shift);

    /* FR-10: TIM1 requires Main Output Enable (MOE) in BDTR for CC outputs */
    if (timer == TIM_ID_1)
    {
        TIM1->BDTR |= TIM_BDTR_MOE;
    }

    return TIM_OK;
}

/* =========================================================================
 * FR-11 – tim_disableTimerCompareChannel
 * Clear the CCxE bit in CCER to deactivate the channel output pin.
 * ========================================================================= */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel)
{
    /* NFR-2: Validate timer and channel */
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) { return st; }

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* FR-11: Clear CCxE to prevent the channel from driving its output pin */
    uint32_t shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER &= ~(1UL << shift);

    return TIM_OK;
}

/* =========================================================================
 * Private helper implementations
 * ========================================================================= */

/**
 * @brief Return the hardware TIM_TypeDef pointer for the given timer ID.
 *
 * Always call prv_validateTimer() first; this function returns NULL for
 * invalid IDs but that case should never reach the caller in normal operation.
 */
static TIM_TypeDef *prv_getTimer(TIM_Id_t timer)
{
    switch (timer)
    {
        case TIM_ID_1:  return TIM1;
        case TIM_ID_2:  return TIM2;
        case TIM_ID_3:  return TIM3;
        case TIM_ID_4:  return TIM4;
        case TIM_ID_5:  return TIM5;
        case TIM_ID_9:  return TIM9;
        case TIM_ID_10: return TIM10;
        case TIM_ID_11: return TIM11;
        default:        return (TIM_TypeDef *)0;
    }
}

/**
 * @brief Validate a timer ID.
 *
 * @return TIM_OK if in range; TIM_ERR_INVALID_TIMER otherwise.
 */
static TIM_Status_t prv_validateTimer(TIM_Id_t timer)
{
    if (timer >= TIM_ID_MAX) { return TIM_ERR_INVALID_TIMER; }
    return TIM_OK;
}

/**
 * @brief Return the maximum number of CC channels available on a timer.
 *
 *   TIM10 / TIM11 → 1 channel
 *   TIM9          → 2 channels
 *   All others    → 4 channels
 */
static uint8_t prv_maxChannels(TIM_Id_t timer)
{
    switch (timer)
    {
        case TIM_ID_10:
        case TIM_ID_11: return 1U;
        case TIM_ID_9:  return 2U;
        default:        return 4U;
    }
}

/**
 * @brief Validate a timer ID and verify the channel is available on it.
 *
 * @return TIM_OK, TIM_ERR_INVALID_TIMER, TIM_ERR_INVALID_CHANNEL, or
 *         TIM_ERR_CHANNEL_UNSUP.
 */
static TIM_Status_t prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) { return st; }

    /* NFR-2: Channel number must be within the global range */
    if (ch < TIM_CH_1 || ch > TIM_CH_4) { return TIM_ERR_INVALID_CHANNEL; }

    /* NFR-2: Channel must be available on the selected timer */
    if ((uint8_t)ch > prv_maxChannels(timer)) { return TIM_ERR_CHANNEL_UNSUP; }

    return TIM_OK;
}
