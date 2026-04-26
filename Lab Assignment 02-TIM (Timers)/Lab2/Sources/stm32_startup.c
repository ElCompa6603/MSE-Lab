#include <stdint.h>
#include "stm32f4xx.h"           /* CMSIS: MCU peripheral definitions (RCC, GPIO, etc.) */
#include "system_stm32f4xx.h"    /* Declares SystemInit()                               */

/* Symbols from linker script */
extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* FPU registers */
#define FPU_CPACR   (*((volatile uint32_t *)0xE000ED88))

/* Function prototypes */
void Default_Handler(void);
void Reset_Handler(void) __attribute__((noreturn));

/* ─── Exception & IRQ Handlers (weak aliases) ─── */
void NMI_Handler                  (void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler            (void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler            (void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler             (void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler           (void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler                  (void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler             (void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler               (void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler              (void) __attribute__((weak, alias("Default_Handler")));
void WWDG_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler               (void) __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler        (void) __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler          (void) __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler               (void) __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler               (void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler     (void) __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler     (void) __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler         (void) __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler         (void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_WKUP_IRQHandler       (void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream7_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void SDIO_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler           (void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler               (void) __attribute__((weak, alias("Default_Handler")));
void SPI4_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void SPI5_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));

/* ─── Type for vector table entries ─── */
typedef void (*const isr_t)(void);

typedef union {
    isr_t    handler;
    uint32_t value;
} vector_entry_t;

/* ─── Vector Table ─── */
vector_entry_t vector_tbl[] __attribute__((section(".isr_vector_tbl"))) = {
    { .value   = (uint32_t)&_estack },
    { .handler = Reset_Handler },
    { .handler = NMI_Handler },
    { .handler = HardFault_Handler },
    { .handler = MemManage_Handler },
    { .handler = BusFault_Handler },
    { .handler = UsageFault_Handler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = SVC_Handler },
    { .handler = DebugMon_Handler },
    { .value   = 0 },
    { .handler = PendSV_Handler },
    { .handler = SysTick_Handler },
    { .handler = WWDG_IRQHandler },
    { .handler = PVD_IRQHandler },
    { .handler = TAMP_STAMP_IRQHandler },
    { .handler = RTC_WKUP_IRQHandler },
    { .handler = FLASH_IRQHandler },
    { .handler = RCC_IRQHandler },
    { .handler = EXTI0_IRQHandler },
    { .handler = EXTI1_IRQHandler },
    { .handler = EXTI2_IRQHandler },
    { .handler = EXTI3_IRQHandler },
    { .handler = EXTI4_IRQHandler },
    { .handler = DMA1_Stream0_IRQHandler },
    { .handler = DMA1_Stream1_IRQHandler },
    { .handler = DMA1_Stream2_IRQHandler },
    { .handler = DMA1_Stream3_IRQHandler },
    { .handler = DMA1_Stream4_IRQHandler },
    { .handler = DMA1_Stream5_IRQHandler },
    { .handler = DMA1_Stream6_IRQHandler },
    { .handler = ADC_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = EXTI9_5_IRQHandler },
    { .handler = TIM1_BRK_TIM9_IRQHandler },
    { .handler = TIM1_UP_TIM10_IRQHandler },
    { .handler = TIM1_TRG_COM_TIM11_IRQHandler },
    { .handler = TIM1_CC_IRQHandler },
    { .handler = TIM2_IRQHandler },
    { .handler = TIM3_IRQHandler },
    { .handler = TIM4_IRQHandler },
    { .handler = I2C1_EV_IRQHandler },
    { .handler = I2C1_ER_IRQHandler },
    { .handler = I2C2_EV_IRQHandler },
    { .handler = I2C2_ER_IRQHandler },
    { .handler = SPI1_IRQHandler },
    { .handler = SPI2_IRQHandler },
    { .handler = USART1_IRQHandler },
    { .handler = USART2_IRQHandler },
    { .value   = 0 },
    { .handler = EXTI15_10_IRQHandler },
    { .handler = RTC_Alarm_IRQHandler },
    { .handler = OTG_FS_WKUP_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = DMA1_Stream7_IRQHandler },
    { .value   = 0 },
    { .handler = SDIO_IRQHandler },
    { .handler = TIM5_IRQHandler },
    { .handler = SPI3_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = DMA2_Stream0_IRQHandler },
    { .handler = DMA2_Stream1_IRQHandler },
    { .handler = DMA2_Stream2_IRQHandler },
    { .handler = DMA2_Stream3_IRQHandler },
    { .handler = DMA2_Stream4_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = OTG_FS_IRQHandler },
    { .handler = DMA2_Stream5_IRQHandler },
    { .handler = DMA2_Stream6_IRQHandler },
    { .handler = DMA2_Stream7_IRQHandler },
    { .handler = USART6_IRQHandler },
    { .handler = I2C3_EV_IRQHandler },
    { .handler = I2C3_ER_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = FPU_IRQHandler },
    { .value   = 0 },
    { .value   = 0 },
    { .handler = SPI4_IRQHandler },
    { .handler = SPI5_IRQHandler }
};

/* ─── Default Handler ─── */
void Default_Handler(void)
{
    while(1);
}

/* ─── Reset Handler ─── */
void Reset_Handler(void)
{
    /* 1. Enable FPU (CP10 and CP11 with full access) */
    FPU_CPACR |= (0xFU << 20);

    /* 2. Calculate .data and .bss sizes in 32-bit words */
    uint32_t data_size = ((uint32_t)&_edata - (uint32_t)&_sdata) / 4;
    uint32_t bss_size  = ((uint32_t)&_ebss  - (uint32_t)&_sbss)  / 4;

    /* 3. Copy .data section from FLASH → SRAM */
    uint32_t *src  = (uint32_t *)&_etext;
    uint32_t *dest = (uint32_t *)&_sdata;
    for (uint32_t i = 0; i < data_size; i++)
        *dest++ = *src++;

    /* 4. Zero-initialize .bss section */
    dest = (uint32_t *)&_sbss;
    for (uint32_t i = 0; i < bss_size; i++)
        *dest++ = 0;

    /* 5. Initialize the clock system (SystemInit configures
          PLL, flash latency, etc. as defined in system_stm32f4xx.c) */
    SystemInit();

    /* 6. Call main() via function pointer so GCC generates
          a correct BLX in Thumb mode without problematic relocations */
    typedef int (*main_fn_t)(void);
    extern int main(void);
    main_fn_t main_ptr = main;
    main_ptr();

    /* 7. Safety trap: if main() ever returns */
    while(1);
}