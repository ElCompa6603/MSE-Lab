#******************************************************************************
# Makefile for Lab5 - I2C/SPI Driver Development and Sensor Integration
# STM32F411RE (Nucleo-64)
# Team 4
#
# Usage:
#   make build   - Compile and link everything into Build/app.elf
#   make flash   - Compile, link and flash to STM32 using OpenOCD
#   make clean   - Remove Objects/ and Build/ folders
#******************************************************************************

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

OBJS  = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))
OBJS += $(addprefix $(OBJ_DIR)/, $(notdir $(filter %.o, $(ASMS:.s=.o))))

vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(ASMS)))

CC         = arm-none-eabi-gcc
DEVICE_DEF = -DSTM32F411xE
CFLAGS     = -g -O0 -std=c99 -Wall $(ARCHFLAGS) $(DEVICE_DEF)
LDFLAGS    = -T $(LINKER_FILE) -lrdimon

.PHONY: build flash clean

build: clean $(EXEC)

flash: clean $(EXEC)
	@echo ""
	@echo "Flashing $(EXEC) to STM32..."
	openocd -f board/st_nucleo_f4.cfg \
	        -c "program $(EXEC) verify reset" \
	        -c "arm semihosting enable" \
	        -c shutdown

$(EXEC): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@
	@echo ""
	@echo "Build complete: $(EXEC)"
	@arm-none-eabi-size $(EXEC)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(OBJ_DIR)/%.o: %.s
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)
