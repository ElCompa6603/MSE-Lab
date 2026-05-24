#******************************************************************************
# Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
#
# Redistribution, modification or use of this software in source or binary
# forms is permitted as long as the files maintain this copyright. Users are
# permitted to modify this and use it to learn about the field of embedded
# software. Carlos Villarreal and CETYS Universidad are not liable for any
# misuse of this material.
#
#******************************************************************************

# List of project source files.
# Adding a new module here is enough for the Makefile to compile it.
# Note: gpio.c and tim.c have been removed — their functionality is
#       provided by gpio_driver.c and tim_driver.c respectively.

SRCS = src/startup.c      \
       src/gpio_driver.c  \
       src/adc.c          \
       src/tim_driver.c   \
       src/pwm.c          \
       src/sensor.c       \
       src/main.c

# Header file search directories
INCLUDES = -I inc             \
           -I CMSIS/STM32F4xx \
           -I CMSIS
