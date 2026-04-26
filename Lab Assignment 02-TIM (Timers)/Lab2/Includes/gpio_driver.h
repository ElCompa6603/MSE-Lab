/**
 ******************************************************************************
 * @file    gpio_driver.h
 * @brief   GPIO Driver – Public API
 *          Compliant with SRS-GPIO_Driver v1.0
 *
 * Functional Requirements:
 *   FR-1  gpio_init       – Configure all ports to default state
 *   FR-2  gpio_initPort   – Enable clock for a specific port
 *   FR-3  gpio_setPinMode – Configure pin direction / mode
 *   FR-4  gpio_setPin     – Drive pin HIGH
 *   FR-5  gpio_clearPin   – Drive pin LOW
 *   FR-6  gpio_togglePin  – Invert current pin state
 *   FR-7  gpio_readPin    – Return digital state of pin
 *
 * Non-Functional Requirements:
 *   NFR-1  Lightweight – direct register access, no HAL dependency
 *   NFR-2  Error handling for invalid port / pin / mode
 *   NFR-3  Real-time safe – no blocking calls, no dynamic allocation
 *
 * Target: STM32F411RE (RM0383)
 ******************************************************************************
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "stm32f4xx.h" 
#include <stdint.h>

/* =========================================================================
 * Enumerations
 * ========================================================================= */

typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_H = 7,
    GPIO_PORT_MAX
} GPIO_Port_t;

typedef enum
{
    GPIO_MODE_INPUT  = 0x00U,
    GPIO_MODE_OUTPUT = 0x01U,
    GPIO_MODE_ALT_FN = 0x02U,
    GPIO_MODE_ANALOG = 0x03U,
    GPIO_MODE_MAX    = 0x04U
} GPIO_Mode_t;

typedef enum
{
    GPIO_PULL_NONE = 0x00U,
    GPIO_PULL_UP   = 0x01U,
    GPIO_PULL_DOWN = 0x02U
} GPIO_Pull_t;

typedef enum
{
    GPIO_OTYPE_PUSH_PULL  = 0x00U,
    GPIO_OTYPE_OPEN_DRAIN = 0x01U
} GPIO_OType_t;

typedef enum
{
    GPIO_SPEED_LOW    = 0x00U,
    GPIO_SPEED_MEDIUM = 0x01U,
    GPIO_SPEED_HIGH   = 0x02U,
    GPIO_SPEED_VHIGH  = 0x03U
} GPIO_Speed_t;

typedef enum
{
    GPIO_PIN_LOW  = 0,
    GPIO_PIN_HIGH = 1
} GPIO_PinState_t;

typedef enum
{
    GPIO_OK                =  0,
    GPIO_ERR_INVALID_PORT  = -1,
    GPIO_ERR_INVALID_PIN   = -2,
    GPIO_ERR_INVALID_MODE  = -3,
    GPIO_ERR_NULL_PTR      = -4,
    GPIO_ERR_INVALID_AF    = -5  
} GPIO_Status_t;

typedef struct
{
    GPIO_Mode_t  mode;
    GPIO_Pull_t  pull;
    GPIO_OType_t otype;
    GPIO_Speed_t speed;
} GPIO_PinCfg_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/* FR-1: System initialization */
void          gpio_init(void);

/* FR-2: Port initialization */
GPIO_Status_t gpio_initPort(GPIO_Port_t port);

/* FR-3: Pin configuration */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg);

/* FR-4, FR-5, FR-6: Output control */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin);
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin);
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin);

/* FR-7: Input sensing */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state);

/* FR-8: Alternate Function configuration */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af);

#endif /* GPIO_DRIVER_H */