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
 * @file        : stm32f4.ld
 *
 * @author      : Your Name
 *
 * @brief       : Linker script para STM32F411RE (Nucleo-64)
 *                      512 Kbytes de memoria Flash
 *                      128 Kbytes de memoria SRAM
 *
 *  Target      : STMicroelectronics STM32F411xE
 *
 * ****************************************************************************
 */

ENTRY(Reset_Handler)

_stack_size = 0x400;   /* 1 KB de pila */
_heap_size  = 0x200;   /* 512 B de heap */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    /* Tabla de vectores — debe ser lo primero en Flash */
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

    /* Valores iniciales de .data guardados en Flash */
    _sidata = LOADADDR(.data);

    /* Variables globales inicializadas */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    /* Variables globales no inicializadas */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;   /* alias para rdimon */
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;     /* alias para rdimon */
    } > SRAM

    /* Heap: justo despues de .bss */
    .heap :
    {
        . = ALIGN(8);
        __end__ = .;             /* alias para rdimon */
        end     = .;             /* alias para rdimon (_sbrk) */
        . = . + _heap_size;
        . = ALIGN(8);
    } > SRAM

    /* Pila: al final de SRAM, crece hacia abajo */
    ._stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
    } > SRAM

    _estack = ORIGIN(SRAM) + LENGTH(SRAM);
}
