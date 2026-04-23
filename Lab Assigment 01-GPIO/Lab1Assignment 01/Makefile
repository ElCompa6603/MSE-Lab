include sources.mk

BUILD_DIR   = Build
EXEC        = $(BUILD_DIR)/app.elf
OBJ_DIR     = Objects
LINKER_FILE = Linker/stm32f4.ld

# Configuración de Hardware
CPU   = cortex-m4
ARCH  = armv7e-m
SPECS = rdimon.specs
FPU   = fpv4-sp-d16

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

# LÓGICA DE OBJETOS: Extrae el nombre del archivo y le pone el prefijo Objects/
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))

# VPATH le dice a Make dónde buscar los archivos .c que declaraste en sources.mk
vpath %.c $(sort $(dir $(SRCS)))

CC = arm-none-eabi-gcc
DEVICE_DEF = -DSTM32F411xE
CFLAGS  = -g -O0 -std=c99 -Wall $(ARCHFLAGS) $(DEVICE_DEF)
LDFLAGS = -nostdlib -T $(LINKER_FILE) -lrdimon

.PHONY : build flash clean

build : $(EXEC)

# Regla para enlazar: crea la carpeta Build si no existe y genera el ejecutable ahí
$(EXEC) : $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@

# Regla para compilar: crea la carpeta si no existe y mete los .o ahí
$(OBJ_DIR)/%.o : %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

flash :
	openocd -f board/st_nucleo_f4.cfg \
			-c "program $(EXEC) verify reset" \
			-c "arm semihosting enable" \
			-c shutdown

clean :
	rm -rf $(OBJ_DIR) $(BUILD_DIR)