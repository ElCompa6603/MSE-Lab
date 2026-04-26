/**
 ******************************************************************************
 * @file    gpio_driver.c
 * @brief   GPIO Driver Implementation
 *          Compliant with SRS-GPIO_Driver v1.0
 * Target:  STM32F411RE (RM0383)
 ******************************************************************************
 */

#include "gpio_driver.h"

static GPIO_TypeDef *prv_getPort(GPIO_Port_t port);
static GPIO_Status_t prv_validate(GPIO_Port_t port, uint8_t pin);

/* =========================================================================
 * FR-1 – gpio_init
 * ========================================================================= */
void gpio_init(void)
{
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOHEN);

    GPIOA->MODER = 0xA8000000UL;
    GPIOB->MODER = 0x00000280UL;
    GPIOC->MODER = 0x00000000UL;
    GPIOD->MODER = 0x00000000UL;
    GPIOE->MODER = 0x00000000UL;
    GPIOH->MODER = 0x00000000UL;
}

/* =========================================================================
 * FR-2 – gpio_initPort
 * ========================================================================= */
GPIO_Status_t gpio_initPort(GPIO_Port_t port)
{
    if (port >= GPIO_PORT_MAX) return GPIO_ERR_INVALID_PORT;
    
    RCC->AHB1ENR |= (1UL << port);
    
    (void)RCC->AHB1ENR; 

    return GPIO_OK;
}

/* =========================================================================
 * FR-3 – gpio_setPinMode
 * ========================================================================= */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg)
{
    if (cfg == (GPIO_PinCfg_t *)0) return GPIO_ERR_NULL_PTR;
    
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    /* MODER */
    GPIOx->MODER   &= ~(0x3UL << (pin * 2U));
    GPIOx->MODER   |=  ((uint32_t)cfg->mode << (pin * 2U));

    /* OTYPER */
    GPIOx->OTYPER  &= ~(0x1UL << pin);
    GPIOx->OTYPER  |=  ((uint32_t)cfg->otype << pin);

    /* OSPEEDR */
    GPIOx->OSPEEDR &= ~(0x3UL << (pin * 2U));
    GPIOx->OSPEEDR |=  ((uint32_t)cfg->speed << (pin * 2U));

    /* PUPDR */
    GPIOx->PUPDR   &= ~(0x3UL << (pin * 2U));
    GPIOx->PUPDR   |=  ((uint32_t)cfg->pull << (pin * 2U));

    return GPIO_OK;
}

/* =========================================================================
 * FR-4 – gpio_setPin (Atomic High)
 * ========================================================================= */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->BSRR = (1UL << pin);
    return GPIO_OK;
}

/* =========================================================================
 * FR-5 – gpio_clearPin (Atomic Low)
 * ========================================================================= */
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->BSRR = (1UL << (pin + 16U));
    return GPIO_OK;
}

/* =========================================================================
 * FR-6 – gpio_togglePin
 * ========================================================================= */
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->ODR ^= (1UL << pin);
    return GPIO_OK;
}

/* =========================================================================
 * FR-7 – gpio_readPin
 * ========================================================================= */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state)
{
    if (state == (GPIO_PinState_t *)0) return GPIO_ERR_NULL_PTR;
    
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    if ((prv_getPort(port)->IDR & (1UL << pin))) {
        *state = GPIO_PIN_HIGH;
    } else {
        *state = GPIO_PIN_LOW;
    }

    return GPIO_OK;
}

/* =========================================================================
 * FR-8 – gpio_setAlternateFunction
 * ========================================================================= */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;
    if (af > 15U) return GPIO_ERR_INVALID_AF;

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    /* AFR[0] is for pins 0-7, AFR[1] for 8-15 */
    uint8_t idx = pin >> 3U; 
    uint8_t shift = (pin & 0x7U) << 2U;

    GPIOx->AFR[idx] &= ~(0xFUL << shift);
    GPIOx->AFR[idx] |=  ((uint32_t)af << shift);

    return GPIO_OK;
}

/* =========================================================================
 * Internal Helpers Implementation
 * ========================================================================= */

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

static GPIO_Status_t prv_validate(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX) return GPIO_ERR_INVALID_PORT;
    if (pin  >  15U)           return GPIO_ERR_INVALID_PIN;
    return GPIO_OK;
}