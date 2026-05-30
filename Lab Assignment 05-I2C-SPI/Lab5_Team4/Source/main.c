/**
 ******************************************************************************
 * @file    main.c
 * @author  Team 4
 * @brief   ADXL345 Accelerometer Application
 *          Reads X/Y/Z acceleration over I2C and prints results via UART.
 *
 *          Hardware connections (STM32F411RE Nucleo-64):
 *            I2C1  — PB8 (SCL), PB9 (SDA)
 *            ADXL345 SDO/ALT ADDRESS → GND  (I2C address = 0x53)
 *            UART2 — PA2 (TX), PA3 (RX)  →  ST-Link VCP (115200 baud, 8N1)
 *
 *          Serial monitor output (one line per sample, ~100 ms period):
 *            X:  +1023 mg  |  Y:   -234 mg  |  Z:  +9800 mg
 ******************************************************************************
 */

#include "stm32f4xx.h"
#include "adxl345_driver.h"
#include "uart.h"
#include "serial.h"

/* ── Simple busy-wait delay ─────────────────────────────────────────────── */
/**
 * @brief  Coarse millisecond delay based on NOP loops at 16 MHz.
 *         Approximately 16000 iterations ≈ 1 ms at 16 MHz (no optimization).
 * @param  ms  Delay duration in milliseconds.
 */
static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0U; i < ms; i++)
    {
        volatile uint32_t t = 16000U;
        while (t--) { __asm("nop"); }
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  MAIN                                                                       */
/* ══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    /* ── Initialize UART (USART2, 115200 baud) — must be first for debug ── */
    serial_init();

    serial_printf("\r\n");
    serial_printf("============================================\r\n");
    serial_printf("  ADXL345 Accelerometer Driver — Team 4   \r\n");
    serial_printf("  STM32F411RE Nucleo-64  |  I2C1 100 kHz  \r\n");
    serial_printf("============================================\r\n");

    /* ── Initialize ADXL345 ─────────────────────────────────────────────── */
    serial_printf("Initializing ADXL345... ");

    ADXL345_Status_t init_status = adxl345_init();

    if (init_status == ADXL345_ID_ERR)
    {
        serial_printf("FAILED — Device ID mismatch.\r\n");
        serial_printf("Check wiring and SDO/ALT ADDRESS pin.\r\n");
        while (1) { }   /* Halt — cannot continue without sensor */
    }
    else if (init_status != ADXL345_OK)
    {
        serial_printf("FAILED — I2C communication error.\r\n");
        serial_printf("Check SDA/SCL connections (PB9/PB8).\r\n");
        while (1) { }   /* Halt */
    }

    serial_printf("OK\r\n");
    serial_printf("Configuration: full-resolution, +/-16g, 100 Hz ODR\r\n");
    serial_printf("--------------------------------------------\r\n");
    serial_printf("  X (mg)    |    Y (mg)    |    Z (mg)   \r\n");
    serial_printf("--------------------------------------------\r\n");

    /* ── Main sampling loop ─────────────────────────────────────────────── */
    ADXL345_Data_t accel;

    while (1)
    {
        ADXL345_Status_t read_status = adxl345_readData(&accel);

        if (read_status == ADXL345_OK)
        {
            /*
             * Print formatted acceleration data.
             * serial_printf supports %d for signed integers.
             * Each axis is shown in milli-g with a fixed-width field.
             */
            serial_printf("  X: %d mg  |  Y: %d mg  |  Z: %d mg\r\n",
                          (int)accel.x_mg,
                          (int)accel.y_mg,
                          (int)accel.z_mg);
        }
        else
        {
            serial_printf("  [READ ERROR — I2C communication failed]\r\n");
        }

        /* Sample at ~10 Hz (100 ms between readings) */
        delay_ms(100U);
    }

    /* Unreachable */
    return 0;
}
