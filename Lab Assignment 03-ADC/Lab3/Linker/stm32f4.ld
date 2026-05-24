/* ******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Carlos Villarreal and CETYS Universidad are not liable for any
 * misuse of this material.
 *
 * ****************************************************************************
 *
 * @file        : stm32f411re.ld
 *
 * @author      : Your Name
 *
 * @brief       : Linker script para STM32F411RE (Nucleo-64)
 *                      512 Kbytes de memoria Flash
 *                      128 Kbytes de memoria SRAM
 *
 *                Ajustar el tamano de heap y stack segun la aplicacion.
 *
 *  Target      : STMicroelectronics STM32F411xE
 *
 * ****************************************************************************
 */

/* Especifica el punto de entrada del firmware */
ENTRY(Reset_Handler)

/* Tamanos de pila y heap */
_stack_size = 0x400;   /* 1 KB de pila */
_heap_size  = 0x200;   /* 512 B de heap (no se usa sin malloc) */

/* ============================================================================
 * MEMORY: describe la memoria fisica del STM32F411RE.
 *
 * Flash (rx)  : codigo y datos de solo lectura.
 *               Inicio: 0x08000000, Tamano: 512 KB.
 *
 * SRAM (rwx)  : variables globales y pila en tiempo de ejecucion.
 *               Inicio: 0x20000000, Tamano: 128 KB.
 * ========================================================================== */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* ============================================================================
 * SECTIONS: ubica cada seccion del binario en la region de memoria correcta.
 * ========================================================================== */
SECTIONS
{
    /* Tabla de vectores — debe ser lo primero en Flash (0x08000000) */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Codigo del programa */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > FLASH

    /* Constantes y strings de solo lectura */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Direccion en Flash donde se guardan los valores iniciales de .data */
    _sidata = LOADADDR(.data);

    /* Variables globales inicializadas (se copian de Flash a SRAM en startup) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;   /* Inicio de .data en SRAM */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;   /* Fin de .data en SRAM */
    } > SRAM AT > FLASH

    /* Variables globales no inicializadas (se ponen a cero en startup) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;    /* Inicio de .bss en SRAM */
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;    /* Fin de .bss en SRAM */
    } > SRAM

    /* Pila: ubicada al final de SRAM, crece hacia abajo */
    ._stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
    } > SRAM

    /* _estack = tope de SRAM = valor inicial del stack pointer */
    _estack = ORIGIN(SRAM) + LENGTH(SRAM);
}
