/**
 ******************************************************************************
 * @file    tim_driver.c
 * @brief   TIM Driver Implementation
 *          Compliant with SRS-TIM_Driver v1.1
 * Target:  STM32F411RE (RM0383)
 *
 * Design notes:
 *  - All timers use direct register access (no HAL/LL).
 *  - PSC and ARR are computed from TIM_SYSCLK_HZ (16 MHz HSI).
 *  - For tim_setTimerMs:  PSC=15999, tick=1ms, ARR = ms-1  (max 65535 ms)
 *  - For tim_setTimerFreq: best-fit PSC+ARR with ARR<=0xFFFF
 *  - TIM1 requires MOE bit for CC output; handled in enable/disable CC.
 ******************************************************************************
 */

#include "tim_driver.h"

/* =========================================================================
 * Private helpers – forward declarations
 * ========================================================================= */
static TIM_TypeDef  *prv_getTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch);
static uint8_t       prv_maxChannels(TIM_Id_t timer);

/* =========================================================================
 * FR-1 – tim_init
 * Reset all supported timers to a known disabled state.
 * ========================================================================= */
void tim_init(void)
{
    /* Enable clocks for every supported timer */
    RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN  |
                     RCC_APB2ENR_TIM9EN  |
                     RCC_APB2ENR_TIM10EN |
                     RCC_APB2ENR_TIM11EN);

    RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN |
                     RCC_APB1ENR_TIM3EN |
                     RCC_APB1ENR_TIM4EN |
                     RCC_APB1ENR_TIM5EN);

    /* Force reset then release to clear all registers */
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
 * Enable clock for a single timer (selective init).
 * ========================================================================= */
TIM_Status_t tim_initTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

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

    /* Short read-back to flush the write-buffer */
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;

    return TIM_OK;
}

/* =========================================================================
 * FR-3 – tim_setTimerMs
 * Configure timer period = ms milliseconds.
 *
 * Strategy:
 *   PSC = (SYSCLK / 1000) - 1  → each tick = 1 ms  (PSC = 15999 for 16 MHz)
 *   ARR = ms - 1
 *   Constraint: ARR <= 0xFFFF (65535 ms max for 16-bit timers)
 *               ARR <= 0xFFFFFFFF for TIM2/TIM5 (32-bit)
 * ========================================================================= */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;
    if (ms == 0U)     return TIM_ERR_INVALID_PARAM;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    uint32_t psc = (TIM_SYSCLK_HZ / 1000UL) - 1UL; /* = 15999 */
    uint32_t arr = ms - 1UL;

    /* 16-bit timers: ARR and PSC must fit in 16 bits */
    if (timer != TIM_ID_2 && timer != TIM_ID_5)
    {
        if (arr > 0xFFFFUL || psc > 0xFFFFUL) return TIM_ERR_INVALID_PARAM;
    }

    TIMx->CR1  &= ~TIM_CR1_CEN;   /* Stop timer before reconfiguring */
    TIMx->PSC   = psc;
    TIMx->ARR   = arr;
    TIMx->CNT   = 0U;
    TIMx->SR   &= ~TIM_SR_UIF;    /* Clear any pending update flag */
    TIMx->EGR  |= TIM_EGR_UG;     /* Force register update (load PSC/ARR) */
    TIMx->SR   &= ~TIM_SR_UIF;    /* Clear UIF set by UG */

    return TIM_OK;
}

/* =========================================================================
 * FR-4 – tim_setTimerFreq
 * Configure timer to generate update events at 'hz' Hz.
 *
 * Strategy:
 *   Find smallest PSC such that ARR = (SYSCLK / ((PSC+1) * hz)) - 1
 *   fits in 16 bits (or 32 bits for TIM2/TIM5).
 *   We iterate PSC from 0 upward until ARR <= 0xFFFF.
 * ========================================================================= */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;
    if (hz == 0U)     return TIM_ERR_INVALID_PARAM;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    uint8_t  is32bit = (timer == TIM_ID_2 || timer == TIM_ID_5);
    uint32_t arr_max = is32bit ? 0xFFFFFFFFUL : 0xFFFFUL;

    uint32_t psc = 0U;
    uint32_t arr = 0U;
    uint8_t  found = 0U;

    /* Search for a valid (PSC, ARR) pair */
    for (psc = 0U; psc <= 0xFFFFUL; psc++)
    {
        uint32_t div = (psc + 1UL) * hz;
        if (div == 0U) continue;
        arr = (TIM_SYSCLK_HZ / div);
        if (arr > 0U) arr -= 1UL;

        if (arr <= arr_max)
        {
            found = 1U;
            break;
        }
    }

    if (!found) return TIM_ERR_INVALID_PARAM;

    TIMx->CR1  &= ~TIM_CR1_CEN;
    TIMx->PSC   = psc;
    TIMx->ARR   = arr;
    TIMx->CNT   = 0U;
    TIMx->SR   &= ~TIM_SR_UIF;
    TIMx->EGR  |= TIM_EGR_UG;
    TIMx->SR   &= ~TIM_SR_UIF;

    return TIM_OK;
}

/* =========================================================================
 * FR-5 – tim_enableTimer
 * Start counting (set CEN bit).
 * ========================================================================= */
TIM_Status_t tim_enableTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    prv_getTimer(timer)->CR1 |= TIM_CR1_CEN;
    return TIM_OK;
}

/* =========================================================================
 * FR-6 – tim_disableTimer
 * Stop counting (clear CEN bit).
 * ========================================================================= */
TIM_Status_t tim_disableTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    prv_getTimer(timer)->CR1 &= ~TIM_CR1_CEN;
    return TIM_OK;
}

/* =========================================================================
 * FR-7 – tim_waitTimer
 * Block until UIF is set (one period elapsed).
 * Timer must already be running (CEN=1).
 * ========================================================================= */
TIM_Status_t tim_waitTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* Wait for Update Interrupt Flag */
    while (!(TIMx->SR & TIM_SR_UIF)) { /* spin */ }

    /* Clear the flag so next call works correctly */
    TIMx->SR &= ~TIM_SR_UIF;

    return TIM_OK;
}

/* =========================================================================
 * FR-8 – tim_setTimerCompareChannelValue
 * Write CCR for the specified channel.
 * ========================================================================= */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

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
 * Set OCxM bits in CCMRx for the specified channel.
 * Also enables output preload (OCxPE) for PWM modes.
 * ========================================================================= */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t          timer,
                                     TIM_Channel_t     channel,
                                     TIM_CompareMode_t mode)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);
    uint32_t ocm = (uint32_t)mode & 0x7UL;

    switch (channel)
    {
        case TIM_CH_1:
            /* CCMR1 bits [6:4] = OC1M, bit 3 = OC1PE */
            TIMx->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_CC1S);
            TIMx->CCMR1 |=  (ocm << 4U) | TIM_CCMR1_OC1PE;
            break;
        case TIM_CH_2:
            /* CCMR1 bits [14:12] = OC2M, bit 11 = OC2PE */
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

    /* Enable auto-reload preload */
    TIMx->CR1 |= TIM_CR1_ARPE;

    return TIM_OK;
}

/* =========================================================================
 * FR-10 – tim_enableTimerCompareChannel
 * Set CCxE bit in CCER to activate channel output.
 * For TIM1 (advanced), also set MOE in BDTR.
 * ========================================================================= */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    /* Each channel occupies 4 bits in CCER; CCxE is bit 0 of that nibble */
    uint32_t shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER |= (1UL << shift);

    /* TIM1 is an advanced-control timer: requires MOE (Main Output Enable) */
    if (timer == TIM_ID_1)
    {
        TIM1->BDTR |= TIM_BDTR_MOE;
    }

    return TIM_OK;
}

/* =========================================================================
 * FR-11 – tim_disableTimerCompareChannel
 * Clear CCxE bit in CCER.
 * ========================================================================= */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);
    uint32_t shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER &= ~(1UL << shift);

    return TIM_OK;
}

/* =========================================================================
 * Private Helper Implementations
 * ========================================================================= */

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

static TIM_Status_t prv_validateTimer(TIM_Id_t timer)
{
    if (timer >= TIM_ID_MAX) return TIM_ERR_INVALID_TIMER;
    return TIM_OK;
}

/**
 * @brief  Returns maximum number of CC channels for a given timer.
 *         TIM9        → 2 channels
 *         TIM10/TIM11 → 1 channel
 *         All others  → 4 channels
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

static TIM_Status_t prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    if (ch < TIM_CH_1 || ch > TIM_CH_4)       return TIM_ERR_INVALID_CHANNEL;
    if ((uint8_t)ch > prv_maxChannels(timer))  return TIM_ERR_CHANNEL_UNSUP;

    return TIM_OK;
}
