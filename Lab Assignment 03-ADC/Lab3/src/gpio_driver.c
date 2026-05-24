/**
 ******************************************************************************
 * @file    gpio_driver.c
 * @brief   GPIO Driver Implementation
 *          Compliant with SRS-GPIO_Driver v1.2
 *
 * @author Team 4
 *
 * Non-Functional Requirements compliance:
 *   NFR-1 (Lightweight and efficient):
 *     - Direct register access only; no HAL or middleware dependencies.
 *     - Atomic set/clear via BSRR to avoid read-modify-write race conditions.
 *     - No dynamic memory allocation.
 *   NFR-2 (Error handling):
 *     - All public functions validate port, pin, and pointer arguments
 *       before accessing any hardware register.
 *   NFR-3 (Real-time performance):
 *     - All functions are O(1) with no blocking calls or unbounded loops.
 *
 * Target: STM32F411RE (RM0383)
 ******************************************************************************
 */

#include "gpio_driver.h"

/* =========================================================================
 * Private helper declarations
 * ========================================================================= */

static GPIO_TypeDef  *prv_getPort(GPIO_Port_t port);
static GPIO_Status_t  prv_validate(GPIO_Port_t port, uint8_t pin);

/* =========================================================================
 * FR-1 – gpio_init
 * Initialize all GPIO ports to their power-on-reset default state.
 * ========================================================================= */
void gpio_init(void)
{
    /* FR-1: Enable clocks for all supported ports */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOHEN);

    /* FR-1: Reset MODER to hardware power-on-reset values (RM0383 Table 26) */
    GPIOA->MODER = 0xA8000000UL;  /* PA13/PA14/PA15 = AF (debug), rest = input */
    GPIOB->MODER = 0x00000280UL;  /* PB3/PB4 = AF (debug), rest = input        */
    GPIOC->MODER = 0x00000000UL;
    GPIOD->MODER = 0x00000000UL;
    GPIOE->MODER = 0x00000000UL;
    GPIOH->MODER = 0x00000000UL;
}

/* =========================================================================
 * FR-2 – gpio_initPort
 * Enable the clock for a single port (selective initialization).
 * ========================================================================= */
GPIO_Status_t gpio_initPort(GPIO_Port_t port)
{
    /* NFR-2: Validate port before touching any register */
    if (port >= GPIO_PORT_MAX) { return GPIO_ERR_INVALID_PORT; }

    /* FR-2: Enable the port clock via AHB1ENR bit position = port index */
    RCC->AHB1ENR |= (1UL << port);

    /* Read-back to flush the write buffer before peripheral access */
    (void)RCC->AHB1ENR;

    return GPIO_OK;
}

/* =========================================================================
 * FR-3 – gpio_setPinMode
 * Configure mode, output type, speed, and pull resistor for one pin.
 * ========================================================================= */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg)
{
    /* NFR-2: Reject NULL configuration pointer */
    if (cfg == (GPIO_PinCfg_t *)0) { return GPIO_ERR_NULL_PTR; }

    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    /* FR-3: Set pin operating mode in MODER (2 bits per pin) */
    GPIOx->MODER   &= ~(0x3UL << (pin * 2U));
    GPIOx->MODER   |=  ((uint32_t)cfg->mode << (pin * 2U));

    /* FR-3: Set output type in OTYPER (1 bit per pin) */
    GPIOx->OTYPER  &= ~(0x1UL << pin);
    GPIOx->OTYPER  |=  ((uint32_t)cfg->otype << pin);

    /* FR-3: Set output speed in OSPEEDR (2 bits per pin) */
    GPIOx->OSPEEDR &= ~(0x3UL << (pin * 2U));
    GPIOx->OSPEEDR |=  ((uint32_t)cfg->speed << (pin * 2U));

    /* FR-3: Set pull-up/pull-down in PUPDR (2 bits per pin) */
    GPIOx->PUPDR   &= ~(0x3UL << (pin * 2U));
    GPIOx->PUPDR   |=  ((uint32_t)cfg->pull << (pin * 2U));

    return GPIO_OK;
}

/* =========================================================================
 * FR-4 – gpio_setPin
 * Drive a pin HIGH atomically using the BSRR set bits [15:0].
 * ========================================================================= */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin)
{
    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    /* FR-4: Atomic high — write to BSRR set half (no read-modify-write needed) */
    prv_getPort(port)->BSRR = (1UL << pin);

    return GPIO_OK;
}

/* =========================================================================
 * FR-5 – gpio_clearPin
 * Drive a pin LOW atomically using the BSRR reset bits [31:16].
 * ========================================================================= */
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin)
{
    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    /* FR-5: Atomic low — write to BSRR reset half (bits 16+pin) */
    prv_getPort(port)->BSRR = (1UL << (pin + 16U));

    return GPIO_OK;
}

/* =========================================================================
 * FR-6 – gpio_togglePin
 * Invert the current output state of a pin via XOR on ODR.
 * ========================================================================= */
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin)
{
    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    /* FR-6: Toggle via XOR on ODR */
    prv_getPort(port)->ODR ^= (1UL << pin);

    return GPIO_OK;
}

/* =========================================================================
 * FR-7 – gpio_readPin
 * Read the digital state of a pin from the IDR register.
 * ========================================================================= */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state)
{
    /* NFR-2: Reject NULL output pointer */
    if (state == (GPIO_PinState_t *)0) { return GPIO_ERR_NULL_PTR; }

    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    /* FR-7: Read IDR and write result to caller's variable */
    *state = (prv_getPort(port)->IDR & (1UL << pin)) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;

    return GPIO_OK;
}

/* =========================================================================
 * FR-8 – gpio_setAlternateFunction
 * Write the AF number into AFR[0] (pins 0-7) or AFR[1] (pins 8-15).
 * ========================================================================= */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af)
{
    /* NFR-2: Validate port and pin */
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) { return status; }

    /* NFR-2: Alternate function numbers are 0-15 only */
    if (af > 15U) { return GPIO_ERR_INVALID_AF; }

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    /* FR-8: AFR[0] controls pins 0-7; AFR[1] controls pins 8-15 (4 bits each) */
    uint8_t idx   = pin >> 3U;          /* 0 for pins 0-7, 1 for pins 8-15 */
    uint8_t shift = (pin & 0x7U) << 2U; /* Bit offset within the register   */

    GPIOx->AFR[idx] &= ~(0xFUL << shift);
    GPIOx->AFR[idx] |=  ((uint32_t)af << shift);

    return GPIO_OK;
}

/* =========================================================================
 * Private helper implementations
 * ========================================================================= */

/**
 * @brief Return the GPIO_TypeDef pointer for the given port.
 *
 * Returns NULL for an unsupported port; callers should always run
 * prv_validate() first so this case never occurs in normal operation.
 */
static GPIO_TypeDef *prv_getPort(GPIO_Port_t port)
{
    switch (port)
    {
        case GPIO_PORT_A: return GPIOA;
        case GPIO_PORT_B: return GPIOB;
        case GPIO_PORT_C: return GPIOC;
        case GPIO_PORT_D: return GPIOD;
        case GPIO_PORT_E: return GPIOE;
        case GPIO_PORT_H: return GPIOH;
        default:          return (GPIO_TypeDef *)0;
    }
}

/**
 * @brief Validate port index and pin number.
 *
 * @return GPIO_OK if both are in range; GPIO_ERR_INVALID_PORT or
 *         GPIO_ERR_INVALID_PIN otherwise.
 */
static GPIO_Status_t prv_validate(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX) { return GPIO_ERR_INVALID_PORT; }
    if (pin  >  15U)           { return GPIO_ERR_INVALID_PIN;  }
    return GPIO_OK;
}
