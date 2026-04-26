include sources.mk

BUILD_DIR   = Build
EXEC        = $(BUILD_DIR)/app.elf
OBJ_DIR     = Objects
LINKER_FILE = Linker/stm32f4.ld

# Hardware configuration – STM32F411RE (Cortex-M4, FPv4-SP)
CPU   = cortex-m4
ARCH  = armv7e-m
SPECS = rdimon.specs
FPU   = fpv4-sp-d16

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

# Objects: strip directory, prepend OBJ_DIR/
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))

# VPATH: let Make find .c files in any source subdirectory
vpath %.c $(sort $(dir $(SRCS)))

CC         = arm-none-eabi-gcc
DEVICE_DEF = -DSTM32F411xE
CFLAGS     = -g -O0 -std=c99 -Wall $(ARCHFLAGS) $(DEVICE_DEF)
LDFLAGS    = -nostdlib -T $(LINKER_FILE) -lrdimon

.PHONY: build flash clean

build: $(EXEC)

$(EXEC): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@
	@echo "==> Build successful: $(EXEC)"

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

flash:
	openocd -f board/st_nucleo_f4.cfg \
		-c "program $(EXEC) verify reset" \
		-c "arm semihosting enable"       \
		-c shutdown

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)
