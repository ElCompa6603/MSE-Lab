/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Carlos Villarreal and CETYS Universidad are not liable for any
 * misuse of this material.
 *
 ******************************************************************************/
/**
 * @file startup.c
 * @brief Codigo de arranque y tabla de vectores para STM32F411RE.
 *
 * Se ejecuta ANTES que main(). Realiza tres tareas:
 *   1. Define la tabla de vectores de interrupcion en la seccion .isr_vector.
 *   2. Copia las variables globales inicializadas de Flash a SRAM (.data).
 *   3. Pone en cero las variables no inicializadas (.bss).
 *   Luego llama a main().
 *
 * @author Your Name
 * @date April 29, 2026
 */

/*** Includes ***/
#include <stdint.h>

/*** External Variables ***/
/* Simbolos definidos por el linker script */
extern uint32_t _estack;   /* Tope de la pila (fin de SRAM) */
extern uint32_t _sdata;    /* Inicio de .data en SRAM */
extern uint32_t _edata;    /* Fin de .data en SRAM */
extern uint32_t _sidata;   /* Fuente de .data en Flash */
extern uint32_t _sbss;     /* Inicio de .bss en SRAM */
extern uint32_t _ebss;     /* Fin de .bss en SRAM */

/*** Function Prototypes ***/
void Reset_Handler(void);
void Default_Handler(void);
int  main(void);

/* Manejadores de excepcion — debiles, apuntan a Default_Handler por defecto */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/*** Tabla de vectores de interrupcion ***/
/* Colocada al inicio de Flash (0x08000000) por el linker script.
 * El Cortex-M4 lee la palabra 0 como el puntero de pila inicial
 * y la palabra 1 como la direccion del Reset_Handler. */
__attribute__((section(".isr_vector")))
uint32_t g_vector_table[] = {
    (uint32_t)&_estack,           /* [0]  Puntero de pila inicial */
    (uint32_t)&Reset_Handler,     /* [1]  Reset: primera funcion que corre */
    (uint32_t)&NMI_Handler,       /* [2]  Non-Maskable Interrupt */
    (uint32_t)&HardFault_Handler, /* [3]  Hard Fault */
    (uint32_t)&MemManage_Handler, /* [4]  Memory Management Fault */
    (uint32_t)&BusFault_Handler,  /* [5]  Bus Fault */
    (uint32_t)&UsageFault_Handler,/* [6]  Usage Fault */
    0, 0, 0, 0,                   /* [7-10] Reservados por ARM */
    (uint32_t)&SVC_Handler,       /* [11] SVCall */
    0, 0,                         /* [12-13] Reservados */
    (uint32_t)&PendSV_Handler,    /* [14] PendSV */
    (uint32_t)&SysTick_Handler,   /* [15] SysTick */
};

/*** Function Definitions ***/

/**
 * @brief Reset Handler — primer codigo C que corre despues del reset.
 *
 * 1. Copia .data de Flash a SRAM.
 * 2. Borra .bss en SRAM.
 * 3. Llama a main().
 *
 * @return None
 */
void Reset_Handler(void)
{
    /* Copiar variables globales inicializadas (ej. int x = 5) de Flash a SRAM */
    uint32_t *src = &_sidata;   /* Fuente en Flash */
    uint32_t *dst = &_sdata;    /* Destino en SRAM */
    while (dst < &_edata) { *dst++ = *src++; }

    /* Limpiar variables no inicializadas (ej. int y) — el estandar C exige que sean 0 */
    uint32_t *bss = &_sbss;
    while (bss < &_ebss) { *bss++ = 0U; }

    /* Llamar a la aplicacion */
    main();

    /* Si main() regresa (no deberia), quedamos en un lazo infinito */
    while (1) {}
}

/**
 * @brief Manejador por defecto para excepciones no implementadas.
 *
 * Detiene el procesador. Un depurador puede pausar aqui para
 * identificar que excepcion se disparo.
 *
 * @return None
 */
void Default_Handler(void)
{
    while (1) {}
}
