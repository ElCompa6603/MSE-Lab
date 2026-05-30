#******************************************************************************
# sources.mk - Source files and includes for Lab5 - I2C/SPI Driver Lab
# STM32F411RE (Nucleo-64)
# Team 4
#******************************************************************************

SRCS = \
    Source/startup.c          \
    Source/gpio_driver.c      \
    Source/uart.c             \
    Source/serial.c           \
    Source/utils.c            \
    Source/i2c_driver.c       \
    Source/spi_driver.c       \
    Source/adxl345_driver.c   \
    Source/main.c

ASMS =

INCLUDES = \
    -I Include           \
    -I CMSIS             \
    -I CMSIS/STM32F4xx
