# sources.mk - GPIO Driver Lab Project

SRC_DIR     = Sources
INC_DIR     = Includes
LINKER_DIR  = Linker

# Source files
SRCS = \
$(SRC_DIR)/main.c             \
$(SRC_DIR)/stm32_startup.c \
$(SRC_DIR)/gpio_driver.c      \
$(SRC_DIR)/led.c              \
$(SRC_DIR)/button.c           \
$(SRC_DIR)/system_stm32f4xx.c

# Ahora apuntamos a donde realmente están los archivos
INCLUDES = \
-I$(INC_DIR) \
-ICMSIS/STM32F4xx \
-ICMSIS