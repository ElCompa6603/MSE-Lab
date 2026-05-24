#******************************************************************************
# sources.mk - Source files and includes for ADC_PWM_Lab
# STM32F411RE (Nucleo-64)
#******************************************************************************

# -----------------------------------------------------------------------------
# SRCS: project .c source files
# Note: gpio.c and tim.c have been removed — their functionality is provided
#       by gpio_driver.c and tim_driver.c respectively.
# -----------------------------------------------------------------------------
SRCS = \
    Source/startup.c      \
    Source/gpio_driver.c  \
    Source/adc.c          \
    Source/tim_driver.c   \
    Source/pwm.c          \
    Source/sensor.c       \
    Source/uart.c         \
    Source/serial.c       \
    Source/utils.c        \
    Source/main.c

# -----------------------------------------------------------------------------
# ASMS: assembler .s files
# The project uses Source/startup.c instead; no .s file is needed.
# -----------------------------------------------------------------------------
ASMS =

# -----------------------------------------------------------------------------
# INCLUDES: header search directories (-I)
# -----------------------------------------------------------------------------
INCLUDES = \
    -I include          \
    -I CMSIS            \
    -I CMSIS/STM32F4xx
