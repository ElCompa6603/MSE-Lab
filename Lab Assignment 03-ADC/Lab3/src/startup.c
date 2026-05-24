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
 * @brief Startup code and interrupt vector table for STM32F411RE.
 *
 * Runs BEFORE main(). Performs three tasks:
 *   1. Defines the interrupt vector table in the .isr_vector section.
 *   2. Copies initialized global variables from Flash to SRAM (.data).
 *   3. Zeros out uninitialized variables (.bss).
 *   Then calls main().
 *
 * @author Team 4
 * @date April 29, 2026
 */

/*** Includes ***/
#include <stdint.h>

/*** External Variables ***/
/* Symbols defined by the linker script */
extern uint32_t _estack;   /* Top of stack (end of SRAM)     */
extern uint32_t _sdata;    /* Start of .data in SRAM         */
extern uint32_t _edata;    /* End of .data in SRAM           */
extern uint32_t _sidata;   /* Source of .data in Flash       */
extern uint32_t _sbss;     /* Start of .bss in SRAM          */
extern uint32_t _ebss;     /* End of .bss in SRAM            */

/*** Function Prototypes ***/
void Reset_Handler(void);
void Default_Handler(void);
int  main(void);

/* Exception handlers — weak aliases pointing to Default_Handler by default */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/*** Interrupt Vector Table ***/
/* Placed at the beginning of Flash (0x08000000) by the linker script.
 * The Cortex-M4 reads word 0 as the initial stack pointer
 * and word 1 as the address of Reset_Handler. */
__attribute__((section(".isr_vector")))
uint32_t g_vector_table[] = {
    (uint32_t)&_estack,           /* [0]  Initial stack pointer        */
    (uint32_t)&Reset_Handler,     /* [1]  Reset: first function to run */
    (uint32_t)&NMI_Handler,       /* [2]  Non-Maskable Interrupt       */
    (uint32_t)&HardFault_Handler, /* [3]  Hard Fault                   */
    (uint32_t)&MemManage_Handler, /* [4]  Memory Management Fault      */
    (uint32_t)&BusFault_Handler,  /* [5]  Bus Fault                    */
    (uint32_t)&UsageFault_Handler,/* [6]  Usage Fault                  */
    0, 0, 0, 0,                   /* [7-10] Reserved by ARM            */
    (uint32_t)&SVC_Handler,       /* [11] SVCall                       */
    0, 0,                         /* [12-13] Reserved                  */
    (uint32_t)&PendSV_Handler,    /* [14] PendSV                       */
    (uint32_t)&SysTick_Handler,   /* [15] SysTick                      */
};

/*** Function Definitions ***/

/**
 * @brief Reset Handler — first C code to run after reset.
 *
 * 1. Copy .data from Flash to SRAM.
 * 2. Zero out .bss in SRAM.
 * 3. Call main().
 *
 * @return None
 */
void Reset_Handler(void)
{
    /* Copy initialized global variables (e.g. int x = 5) from Flash to SRAM */
    uint32_t *src = &_sidata;   /* Source in Flash      */
    uint32_t *dst = &_sdata;    /* Destination in SRAM  */
    while (dst < &_edata) { *dst++ = *src++; }

    /* Zero out uninitialized variables (e.g. int y) — the C standard requires them to be 0 */
    uint32_t *bss = &_sbss;
    while (bss < &_ebss) { *bss++ = 0U; }

    /* Call the application */
    main();

    /* If main() returns (it shouldn't), loop forever */
    while (1) {}
}

/**
 * @brief Default handler for unimplemented exceptions.
 *
 * Stalls the processor. A debugger can halt here to identify
 * which exception was triggered.
 *
 * @return None
 */
void Default_Handler(void)
{
    while (1) {}
}
