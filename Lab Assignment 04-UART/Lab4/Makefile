#******************************************************************************
# Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
#******************************************************************************

#------------------------------------------------------------------------------
# Makefile for the ADC_PWM_Lab build system
#
# Usage: make [TARGET]
#   build   - Cleans, compiles and links everything into Build/app.elf
#   flash   - Cleans, compiles, links and flashes to the STM32 using OpenOCD
#   clean   - Removes the Objects/ folder and the Build/ folder
#
# Platform:
#   STM32F411RE (Nucleo-64) with arm-none-eabi-gcc
#------------------------------------------------------------------------------

include sources.mk

EXEC        = Build/app.elf
OBJ_DIR     = Objects
BUILD_DIR   = Build
LINKER_FILE = Linker/stm32f4.ld

CPU   = cortex-m4
ARCH  = armv7e-m
SPECS = rdimon.specs
FPU   = fpv4-sp-d16

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

# OBJECT FILE LOGIC:
# notdir    -> strips the path       (Source/gpio_driver.c -> gpio_driver.c)
# .c=.o     -> changes the extension (gpio_driver.c -> gpio_driver.o)
# addprefix -> prepends the folder   (gpio_driver.o -> Objects/gpio_driver.o)
OBJS  = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))
OBJS += $(addprefix $(OBJ_DIR)/, $(notdir $(filter %.o, $(ASMS:.s=.o))))

# VPATH tells Make where to find the .c and .s files declared in sources.mk
vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(ASMS)))

CC         = arm-none-eabi-gcc
DEVICE_DEF = -DSTM32F411xE
CFLAGS     = -g -O0 -std=c99 -Wall $(ARCHFLAGS) $(DEVICE_DEF)
LDFLAGS    = -T $(LINKER_FILE) -lrdimon

.PHONY : build flash clean

# build: always starts clean, then compiles and links
build : clean $(EXEC)

# flash: always starts clean, then compiles, links and flashes
flash : clean $(EXEC)
	@echo ""
	@echo "Flashing $(EXEC) to STM32..."
	openocd -f board/st_nucleo_f4.cfg \
	        -c "program $(EXEC) verify reset" \
	        -c "arm semihosting enable" \
	        -c shutdown

# Link rule: creates Build/ and produces Build/app.elf
$(EXEC) : $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@
	@echo ""
	@echo "Build complete: $(EXEC)"
	@arm-none-eabi-size $(EXEC)

# Compile rule: creates Objects/ and generates each .o
$(OBJ_DIR)/%.o : %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(OBJ_DIR)/%.o : %.s
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) -o $@

# clean: removes Objects/ and Build/
clean :
	rm -rf $(OBJ_DIR) $(BUILD_DIR)