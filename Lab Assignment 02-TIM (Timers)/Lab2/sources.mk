# sources.mk – TIM Driver Lab (Lab 2)

SRC_DIR    = Sources
INC_DIR    = Includes
LINKER_DIR = Linker

# Source files
SRCS = \
$(SRC_DIR)/main.c             \
$(SRC_DIR)/stm32_startup.c    \
$(SRC_DIR)/system_stm32f4xx.c \
$(SRC_DIR)/gpio_driver.c      \
$(SRC_DIR)/tim_driver.c       \
$(SRC_DIR)/timer.c            \
$(SRC_DIR)/pwm.c              \
$(SRC_DIR)/led.c

# Include directories
INCLUDES = \
-I$(INC_DIR)          \
-ICMSIS/STM32F4xx     \
-ICMSIS
