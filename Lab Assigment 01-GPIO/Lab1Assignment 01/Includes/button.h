/**
 ******************************************************************************
 * @file    button.h
 * @brief   Button Module – Nucleo-64 USER button (PC13, active LOW)
 *          Uses gpio_driver internally (Task 3)
 ******************************************************************************
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "gpio_driver.h"

#define BUTTON_PORT     GPIO_PORT_C
#define BUTTON_PIN      13U

typedef enum
{
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED  = 1
} ButtonState_t;

void          button_init(void);
ButtonState_t button_get_state(void);

#endif /* BUTTON_H */