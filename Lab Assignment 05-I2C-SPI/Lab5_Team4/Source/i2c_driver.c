/**
 ******************************************************************************
 * @file    i2c_driver.c
 * @author  Team 4
 * @brief   I2C Driver — Implementation
 *          STM32F411RE, I2C1, Standard Mode 100 kHz
 *          PB8 = SCL   PB9 = SDA   (AF4, open-drain)
 ******************************************************************************
 */

#include "i2c_driver.h"
#include "stm32f4xx.h"

/** Mask covering all hardware error flags in SR1 */
#define I2C_ERROR_FLAGS  (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF)

/* ══════════════════════════════════════════════════════════════════════════ */
/*  PRIVATE HELPERS                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Busy-loop delay using NOP instructions.
 *         Does not rely on any timer peripheral.
 * @param  n  Number of loop iterations.
 */
static void prv_delay_cycles(uint32_t n)
{
    volatile uint32_t i = n;
    while (i--) { __asm("nop"); }
}

/**
 * @brief  Waits until a flag (or set of flags) is set in SR1, with timeout.
 * @param  mask  Bit mask of the SR1 flag(s) to wait for.
 * @retval 1  Flag(s) set within timeout.
 * @retval 0  Timeout expired or a hardware error flag was detected.
 */
static uint8_t prv_wait_sr1(uint32_t mask)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & mask))
    {
        if (--t == 0U)                    return 0U;
        if (I2C1->SR1 & I2C_ERROR_FLAGS)  return 0U;
    }
    return 1U;
}

/**
 * @brief  Checks for hardware error flags in SR1 and clears them.
 * @retval 1  At least one error flag was set (flags have been cleared).
 * @retval 0  No error flags were set.
 */
static uint8_t prv_has_error(void)
{
    if (I2C1->SR1 & I2C_ERROR_FLAGS)
    {
        I2C1->SR1 &= ~I2C_ERROR_FLAGS;
        return 1U;
    }
    return 0U;
}

/**
 * @brief  Generates an I2C STOP condition and waits for the bus to be
 *         released (BUSY bit cleared).
 * @note   If the bus does not become free within I2C_TIMEOUT cycles,
 *         i2c_recover() is called automatically.
 */
static void prv_stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    uint32_t t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t) { }

    if (t == 0U)
    {
        /* Bus still busy after STOP — attempt hardware recovery */
        i2c_recover();
    }
}

/**
 * @brief  Verifies that the I2C bus is free before starting a transaction.
 *         If the bus is busy, waits up to I2C_TIMEOUT cycles.
 *         If still busy after the timeout, calls i2c_recover().
 * @retval I2C_OK    Bus is free and ready.
 * @retval I2C_BUSY  Bus could not be acquired even after recovery.
 */
static I2C_Status_t prv_check_bus(void)
{
    if (!(I2C1->SR2 & I2C_SR2_BUSY)) return I2C_OK;

    uint32_t t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t) { }

    if (t == 0U)
    {
        /* Bus still busy after timeout — attempt recovery */
        i2c_recover();

        /* Check one more time after recovery */
        if (I2C1->SR2 & I2C_SR2_BUSY) return I2C_BUSY;
    }

    return I2C_OK;
}

/**
 * @brief  Generates an I2C START condition and waits for the SB flag.
 * @retval I2C_OK   START generated successfully.
 * @retval I2C_ERR  Timeout waiting for SB flag.
 */
static I2C_Status_t prv_start(void)
{
    I2C1->CR1 |= I2C_CR1_START;
    if (!prv_wait_sr1(I2C_SR1_SB)) return I2C_ERR;
    return I2C_OK;
}

/**
 * @brief  Sends the 7-bit device address with the R/W bit and waits for
 *         ADDR flag, then clears it by reading SR1 and SR2.
 * @param  addr7  7-bit device address.
 * @param  read   0 for write, 1 for read.
 * @retval I2C_OK   Address acknowledged by the slave.
 * @retval I2C_ERR  Timeout, NACK, or hardware error.
 */
static I2C_Status_t prv_send_addr(uint8_t addr7, uint8_t read)
{
    I2C1->DR = (uint8_t)((addr7 << 1U) | (read & 1U));
    if (!prv_wait_sr1(I2C_SR1_ADDR)) return I2C_ERR;
    if (prv_has_error())              return I2C_ERR;
    /* Clear ADDR flag by reading SR1 then SR2 */
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return I2C_OK;
}

/**
 * @brief  Waits for TXE (transmit data register empty) then writes one byte.
 * @param  byte  Byte to transmit.
 * @retval I2C_OK   Byte written to DR successfully.
 * @retval I2C_ERR  Timeout waiting for TXE.
 */
static I2C_Status_t prv_write_byte(uint8_t byte)
{
    if (!prv_wait_sr1(I2C_SR1_TXE)) return I2C_ERR;
    I2C1->DR = byte;
    return I2C_OK;
}

/**
 * @brief  Waits for BTF (byte transfer finished) flag.
 *         Ensures the last byte has been fully shifted out before STOP.
 * @retval I2C_OK   BTF set; transfer complete.
 * @retval I2C_ERR  Timeout waiting for BTF.
 */
static I2C_Status_t prv_wait_btf(void)
{
    if (!prv_wait_sr1(I2C_SR1_BTF)) return I2C_ERR;
    return I2C_OK;
}

/**
 * @brief  Validates a 7-bit I2C device address against the reserved ranges
 *         defined by the I2C specification.
 * @param  addr7  Address to validate.
 * @retval 1  Address is valid (0x08–0x77).
 * @retval 0  Address is reserved or out of range.
 */
static uint8_t prv_is_valid_addr(uint8_t addr7)
{
    return (addr7 >= I2C_ADDR_MIN && addr7 <= I2C_ADDR_MAX) ? 1U : 0U;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  BUS RECOVERY                                                               */
/* ══════════════════════════════════════════════════════════════════════════ */

void i2c_recover(void)
{
    /* Step 1 — Reconfigure SCL (PB8) and SDA (PB9) as GPIO output open-drain */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* PB8 — output, open-drain, high speed */
    GPIOB->MODER   = (GPIOB->MODER   & ~(3UL << 16U)) | (1UL << 16U);
    GPIOB->OTYPER  |=  (1UL << 8U);
    GPIOB->OSPEEDR |=  (3UL << 16U);

    /* PB9 — output, open-drain, high speed */
    GPIOB->MODER   = (GPIOB->MODER   & ~(3UL << 18U)) | (1UL << 18U);
    GPIOB->OTYPER  |=  (1UL << 9U);
    GPIOB->OSPEEDR |=  (3UL << 18U);

    /* Release both lines HIGH */
    GPIOB->BSRR = (1UL << 8U) | (1UL << 9U);
    prv_delay_cycles(500U);

    /* Step 2 — Toggle SCL up to 9 times to free a slave stuck mid-byte */
    for (uint32_t i = 0U; i < 9U; i++)
    {
        /* Stop early if SDA is released by the slave */
        if (GPIOB->IDR & (1UL << 9U)) break;

        GPIOB->BSRR = (1UL << (8U + 16U));  /* SCL LOW  */
        prv_delay_cycles(500U);
        GPIOB->BSRR = (1UL << 8U);          /* SCL HIGH */
        prv_delay_cycles(500U);
    }

    /* Step 3 — Generate a manual STOP: SDA goes HIGH while SCL is HIGH */
    GPIOB->BSRR = (1UL << (9U + 16U));  /* SDA LOW  */
    prv_delay_cycles(500U);
    GPIOB->BSRR = (1UL << 9U);          /* SDA HIGH */
    prv_delay_cycles(500U);

    /* Step 4 — Software reset of the I2C peripheral */
    I2C1->CR1 |=  I2C_CR1_SWRST;
    prv_delay_cycles(100U);
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* Step 5 — Reconfigure pins as AF4 and re-initialize the peripheral.
     *
     * WARNING (OBS-2): gpio_initPort() called inside i2c_init() initialises
     * the GPIOB port driver.  If other GPIOB pins (e.g. PB0–PB7, PB10+) are
     * in use by the application, verify that gpio_initPort() is idempotent
     * and does NOT reset those pins to their default state.  If it does, move
     * the gpio_initPort() call to system startup (before any GPIOB pin is
     * configured) and remove it from i2c_init() to prevent side-effects here.
     */
    i2c_init();
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  INITIALIZATION                                                             */
/* ══════════════════════════════════════════════════════════════════════════ */

void i2c_init(void)
{
    /* Enable RCC clocks for GPIOB and I2C1; dummy reads ensure clock is active */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    /* Configure PB8 (SCL) and PB9 (SDA) as AF4, open-drain, high speed */
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ALT_FN;
    cfg.otype = GPIO_OTYPE_OPEN_DRAIN;
    cfg.speed = GPIO_SPEED_HIGH;
    cfg.pull  = GPIO_PULL_NONE;

    gpio_initPort(I2C_SCL_PORT);
    gpio_setPinMode(I2C_SCL_PORT, I2C_SCL_PIN, &cfg);
    gpio_setAlternateFunction(I2C_SCL_PORT, I2C_SCL_PIN, I2C_AF);
    gpio_setPinMode(I2C_SDA_PORT, I2C_SDA_PIN, &cfg);
    gpio_setAlternateFunction(I2C_SDA_PORT, I2C_SDA_PIN, I2C_AF);

    /* Software reset to clear any previous peripheral state */
    I2C1->CR1 |=  I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* Configure timing registers for Standard Mode 100 kHz at 16 MHz APB1 */
    I2C1->CR2    = I2C_CR2_FREQ_MHZ & I2C_CR2_FREQ;
    I2C1->CCR    = I2C_CCR_SM_100K  & I2C_CCR_CCR;
    I2C1->TRISE  = I2C_TRISE_SM;

    /* Enable the I2C peripheral */
    I2C1->CR1   |= I2C_CR1_PE;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  DIRECT WRITE  (used by lcd.c and similar drivers)                         */
/* ══════════════════════════════════════════════════════════════════════════ */

I2C_Status_t i2c_writeDevice(uint8_t dev_addr,
                              const uint8_t *data, uint32_t len)
{
    /* Validate arguments: address range, non-null buffer, non-zero length */
    if (!prv_is_valid_addr(dev_addr))  return I2C_ERR;
    if (data == NULL || len == 0U)     return I2C_ERR;

    if (prv_check_bus()                      != I2C_OK) return I2C_BUSY;
    if (prv_start()                          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U)          != I2C_OK) { prv_stop(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (prv_write_byte(data[i])          != I2C_OK) { prv_stop(); return I2C_ERR; }
    }

    if (prv_wait_btf()                       != I2C_OK) { prv_stop(); return I2C_ERR; }

    prv_stop();
    return I2C_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  REGISTER WRITE                                                             */
/* ══════════════════════════════════════════════════════════════════════════ */

I2C_Status_t i2c_writeRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                 const uint8_t *data, uint32_t len)
{
    if (!prv_is_valid_addr(dev_addr))  return I2C_ERR;
    if (data == NULL || len == 0U)     return I2C_ERR;

    if (prv_check_bus()                      != I2C_OK) return I2C_BUSY;
    if (prv_start()                          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U)          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_write_byte(reg_addr)             != I2C_OK) { prv_stop(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (prv_write_byte(data[i])          != I2C_OK) { prv_stop(); return I2C_ERR; }
    }

    if (prv_wait_btf()                       != I2C_OK) { prv_stop(); return I2C_ERR; }
    prv_stop();
    return I2C_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  REGISTER READ                                                              */
/* ══════════════════════════════════════════════════════════════════════════ */

I2C_Status_t i2c_readRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                uint8_t *data, uint32_t len)
{
    if (!prv_is_valid_addr(dev_addr))  return I2C_ERR;
    if (data == NULL || len == 0U)     return I2C_ERR;

    if (prv_check_bus()                      != I2C_OK) return I2C_BUSY;

    /* Write phase: send device address + register pointer */
    if (prv_start()                          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U)          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_write_byte(reg_addr)             != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_wait_btf()                       != I2C_OK) { prv_stop(); return I2C_ERR; }

    /* Repeated START to switch to read mode */
    if (prv_start()                          != I2C_OK) { prv_stop(); return I2C_ERR; }

    /* ── Single-byte read ───────────────────────────────────────────────── */
    if (len == 1U)
    {
        /* Disable ACK before sending address so NACK is sent after the byte */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (prv_send_addr(dev_addr, 1U)      != I2C_OK) { prv_stop(); return I2C_ERR; }

        /* Program STOP before reading DR to satisfy RM0383 single-byte seq. */
        I2C1->CR1 |= I2C_CR1_STOP;

        /* Wait for the byte to be received before reading DR */
        if (!prv_wait_sr1(I2C_SR1_RXNE))                { return I2C_ERR; }
        data[0] = (uint8_t)I2C1->DR;

        /* Re-enable ACK for future transactions */
        I2C1->CR1 |= I2C_CR1_ACK;
        return I2C_OK;
    }

    /* ── Multi-byte read ────────────────────────────────────────────────── */
    /*
     * Enable ACK for all bytes except the last two (RM0383 Fig. 164).
     *
     * NOTE — len == 2 edge case:
     *   When len == 2, the loop body hits i == (len-2) == 0 on the very
     *   first iteration, so it immediately waits for BTF, clears ACK, and
     *   reads byte 0.  On the next iteration (i == 1 == len-1) it programs
     *   STOP and reads byte 1.  There are no "middle" iterations.
     *   This is correct and matches RM0383 Fig. 164 (2-byte receive sequence).
     */
    I2C1->CR1 |= I2C_CR1_ACK;
    if (prv_send_addr(dev_addr, 1U)          != I2C_OK) { prv_stop(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (i == (len - 2U))
        {
            /*
             * Second-to-last byte (also the first byte when len == 2):
             * Wait for BTF (both DR and shift register are full), then disable
             * ACK so the NACK is sent after the next (last) byte is received.
             */
            if (!prv_wait_sr1(I2C_SR1_BTF))             { prv_stop(); return I2C_ERR; }
            I2C1->CR1 &= ~I2C_CR1_ACK;
            data[i] = (uint8_t)I2C1->DR;
        }
        else if (i == (len - 1U))
        {
            /*
             * Last byte:
             * BTF was already set when we read the second-to-last byte, so DR
             * already holds the last byte. Program STOP, wait for RXNE to
             * confirm DR is readable, then read it.
             */
            I2C1->CR1 |= I2C_CR1_STOP;
            if (!prv_wait_sr1(I2C_SR1_RXNE))            { return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;

            /* Re-enable ACK for future transactions */
            I2C1->CR1 |= I2C_CR1_ACK;
        }
        else
        {
            /* Middle bytes (only reached when len > 2): wait for RXNE and read normally */
            if (!prv_wait_sr1(I2C_SR1_RXNE))            { prv_stop(); return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;
        }
    }
    return I2C_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  DIRECT READ                                                                */
/* ══════════════════════════════════════════════════════════════════════════ */

I2C_Status_t i2c_readDevice(uint8_t dev_addr,
                             uint8_t *data, uint32_t len)
{
    if (!prv_is_valid_addr(dev_addr))  return I2C_ERR;
    if (data == NULL || len == 0U)     return I2C_ERR;

    if (prv_check_bus()                      != I2C_OK) return I2C_BUSY;

    /* ── Single-byte read ───────────────────────────────────────────────── */
    if (len == 1U)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (prv_start()                      != I2C_OK) { prv_stop(); return I2C_ERR; }
        if (prv_send_addr(dev_addr, 1U)      != I2C_OK) { prv_stop(); return I2C_ERR; }

        I2C1->CR1 |= I2C_CR1_STOP;

        if (!prv_wait_sr1(I2C_SR1_RXNE))                { return I2C_ERR; }
        data[0] = (uint8_t)I2C1->DR;

        I2C1->CR1 |= I2C_CR1_ACK;
        return I2C_OK;
    }

    /* ── Multi-byte read ────────────────────────────────────────────────── */
    /*
     * NOTE — len == 2 edge case:
     *   Same reasoning as i2c_readRegDevice: when len == 2, the loop hits
     *   i == (len-2) == 0 immediately (BTF + NACK), then i == 1 (STOP + read).
     *   There are no "middle" iterations.  Correct per RM0383 Fig. 164.
     */
    I2C1->CR1 |= I2C_CR1_ACK;
    if (prv_start()                          != I2C_OK) { prv_stop(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 1U)          != I2C_OK) { prv_stop(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (i == (len - 2U))
        {
            /*
             * Second-to-last byte (also the first byte when len == 2):
             * Wait for BTF, disable ACK so NACK is sent after the last byte.
             */
            if (!prv_wait_sr1(I2C_SR1_BTF))             { prv_stop(); return I2C_ERR; }
            I2C1->CR1 &= ~I2C_CR1_ACK;
            data[i] = (uint8_t)I2C1->DR;
        }
        else if (i == (len - 1U))
        {
            /*
             * Last byte:
             * DR already holds the last byte (BTF was set above). Program STOP,
             * wait for RXNE to confirm DR is readable, then read it.
             */
            I2C1->CR1 |= I2C_CR1_STOP;
            if (!prv_wait_sr1(I2C_SR1_RXNE))            { return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;

            I2C1->CR1 |= I2C_CR1_ACK;
        }
        else
        {
            /* Middle bytes (only reached when len > 2): wait for RXNE and read normally */
            if (!prv_wait_sr1(I2C_SR1_RXNE))            { prv_stop(); return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;
        }
    }
    return I2C_OK;
}