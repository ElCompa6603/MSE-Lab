/**
 ******************************************************************************
 * @file    gpio_driver.h
 * @brief   GPIO Driver - Public API
 *          Compliant with SRS-GPIO_Driver v1.2
 *
 * @author Team 4
 *
 * Functional Requirements:
 *   FR-1  gpio_init                – Configure all ports to default state
 *   FR-2  gpio_initPort            – Enable clock for a specific port
 *   FR-3  gpio_setPinMode          – Configure pin direction / mode
 *   FR-4  gpio_setPin              – Drive pin HIGH
 *   FR-5  gpio_clearPin            – Drive pin LOW
 *   FR-6  gpio_togglePin           – Invert current pin state
 *   FR-7  gpio_readPin             – Return digital state of pin
 *   FR-8  gpio_setAlternateFunction– Configure pin alternate function
 *
 * Non-Functional Requirements:
 *   NFR-1  Lightweight  – direct register access, no HAL dependency
 *   NFR-2  Error handling for invalid port / pin / mode / AF inputs
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

/** @brief Supported GPIO ports. Values match AHB1ENR bit positions. */
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

/** @brief Pin operating modes written to MODER register (2 bits per pin). */
typedef enum
{
    GPIO_MODE_INPUT  = 0x00U,  /* Digital input                          */
    GPIO_MODE_OUTPUT = 0x01U,  /* Digital output                         */
    GPIO_MODE_ALT_FN = 0x02U,  /* Alternate function (UART, SPI, TIM...) */
    GPIO_MODE_ANALOG = 0x03U,  /* Analog mode (ADC/DAC)                  */
    GPIO_MODE_MAX    = 0x04U
} GPIO_Mode_t;

/** @brief Pull-up / pull-down resistor configuration. */
typedef enum
{
    GPIO_PULL_NONE = 0x00U,  /* No pull resistor */
    GPIO_PULL_UP   = 0x01U,  /* Internal pull-up */
    GPIO_PULL_DOWN = 0x02U   /* Internal pull-down */
} GPIO_Pull_t;

/** @brief Output driver type. */
typedef enum
{
    GPIO_OTYPE_PUSH_PULL  = 0x00U,  /* Push-pull output  */
    GPIO_OTYPE_OPEN_DRAIN = 0x01U   /* Open-drain output */
} GPIO_OType_t;

/** @brief Output slew-rate speed. */
typedef enum
{
    GPIO_SPEED_LOW    = 0x00U,  /* Low speed    */
    GPIO_SPEED_MEDIUM = 0x01U,  /* Medium speed */
    GPIO_SPEED_HIGH   = 0x02U,  /* High speed   */
    GPIO_SPEED_VHIGH  = 0x03U   /* Very high speed */
} GPIO_Speed_t;

/** @brief Digital pin state returned by gpio_readPin(). */
typedef enum
{
    GPIO_PIN_LOW  = 0,
    GPIO_PIN_HIGH = 1
} GPIO_PinState_t;

/** @brief Driver return codes (NFR-2: error handling). */
typedef enum
{
    GPIO_OK                =  0,   /* Operation successful            */
    GPIO_ERR_INVALID_PORT  = -1,   /* Port out of range               */
    GPIO_ERR_INVALID_PIN   = -2,   /* Pin number > 15                 */
    GPIO_ERR_INVALID_MODE  = -3,   /* Mode value out of range         */
    GPIO_ERR_NULL_PTR      = -4,   /* NULL pointer passed as argument */
    GPIO_ERR_INVALID_AF    = -5    /* Alternate function number > 15  */
} GPIO_Status_t;

/** @brief Pin configuration structure passed to gpio_setPinMode(). */
typedef struct
{
    GPIO_Mode_t  mode;   /* Pin operating mode   */
    GPIO_Pull_t  pull;   /* Pull resistor setting */
    GPIO_OType_t otype;  /* Output driver type   */
    GPIO_Speed_t speed;  /* Output slew rate     */
} GPIO_PinCfg_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief FR-1: Initialize all GPIO ports to a default state.
 *
 * Enables clocks for all supported ports and resets each MODER
 * register to the power-on-reset value defined in the reference manual.
 */
void gpio_init(void);

/**
 * @brief FR-2: Enable the clock for a specific GPIO port.
 *
 * @param port  Port to initialize (GPIO_PORT_A to GPIO_PORT_H).
 * @return GPIO_OK, or GPIO_ERR_INVALID_PORT if port is out of range.
 */
GPIO_Status_t gpio_initPort(GPIO_Port_t port);

/**
 * @brief FR-3: Configure the mode and electrical characteristics of a pin.
 *
 * @param port  Target GPIO port.
 * @param pin   Pin number (0-15).
 * @param cfg   Pointer to pin configuration structure (must not be NULL).
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, GPIO_ERR_INVALID_PIN, or
 *         GPIO_ERR_NULL_PTR.
 */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg);

/**
 * @brief FR-4: Drive a pin to logic HIGH atomically via BSRR.
 *
 * @param port  Target GPIO port.
 * @param pin   Pin number (0-15).
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, or GPIO_ERR_INVALID_PIN.
 */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin);

/**
 * @brief FR-5: Drive a pin to logic LOW atomically via BSRR.
 *
 * @param port  Target GPIO port.
 * @param pin   Pin number (0-15).
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, or GPIO_ERR_INVALID_PIN.
 */
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin);

/**
 * @brief FR-6: Invert the current output state of a pin via ODR XOR.
 *
 * @param port  Target GPIO port.
 * @param pin   Pin number (0-15).
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, or GPIO_ERR_INVALID_PIN.
 */
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin);

/**
 * @brief FR-7: Read the current digital state of a pin from IDR.
 *
 * @param port   Target GPIO port.
 * @param pin    Pin number (0-15).
 * @param state  Output parameter; set to GPIO_PIN_HIGH or GPIO_PIN_LOW.
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, GPIO_ERR_INVALID_PIN, or
 *         GPIO_ERR_NULL_PTR.
 */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state);

/**
 * @brief FR-8: Configure the alternate function for a pin (AFR register).
 *
 * The pin mode must already be set to GPIO_MODE_ALT_FN via gpio_setPinMode().
 *
 * @param port  Target GPIO port.
 * @param pin   Pin number (0-15).
 * @param af    Alternate function number (0-15).
 * @return GPIO_OK, GPIO_ERR_INVALID_PORT, GPIO_ERR_INVALID_PIN, or
 *         GPIO_ERR_INVALID_AF.
 */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af);

#endif /* GPIO_DRIVER_H */
