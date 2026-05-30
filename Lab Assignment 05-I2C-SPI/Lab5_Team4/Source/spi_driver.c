/**
 ******************************************************************************
 * @file    spi_driver.c
 * @author  Team 4
 * @brief   SPI Driver — Implementation
 *          STM32F411RE, SPI1, Master Mode, 8-bit, Mode 0 (CPOL=0, CPHA=0)
 *          PA5 = SCK   PA6 = MISO   PA7 = MOSI   PA4 = CS   (AF5)
 ******************************************************************************
 */

#include "spi_driver.h"
#include "stm32f4xx.h"

/* ══════════════════════════════════════════════════════════════════════════ */
/*  PRIVATE HELPERS                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Waits until a flag (or set of flags) is set in SR, with timeout.
 * @param  mask  Bit mask of the SR flag(s) to wait for.
 * @retval 1  Flag(s) set within timeout.
 * @retval 0  Timeout expired or a hardware error flag was detected.
 */
static uint8_t prv_wait_sr(uint32_t mask)
{
    uint32_t t = SPI_TIMEOUT;
    while (!(SPI1->SR & mask))
    {
        if (--t == 0U)                   return 0U;
        if (SPI1->SR & SPI_ERROR_FLAGS)  return 0U;
    }
    return 1U;
}

/**
 * @brief  Checks for hardware error flags in SR and clears them.
 *         OVR is cleared by reading DR then SR.
 *         MODF is cleared by reading SR then writing CR1.
 * @retval 1  At least one error flag was set (flags have been cleared).
 * @retval 0  No error flags detected.
 */
static uint8_t prv_has_error(void)
{
    uint32_t sr = SPI1->SR;

    if (!(sr & SPI_ERROR_FLAGS)) return 0U;

    /* Clear OVR: read DR, then read SR */
    if (sr & SPI_SR_OVR)
    {
        (void)SPI1->DR;
        (void)SPI1->SR;
    }

    /* Clear MODF: read SR (already done), then write CR1 */
    if (sr & SPI_SR_MODF)
    {
        SPI1->CR1 = SPI1->CR1;
    }

    /* Clear CRCERR: write 0 to the CRCERR bit */
    if (sr & SPI_SR_CRCERR)
    {
        SPI1->SR &= ~SPI_SR_CRCERR;
    }

    return 1U;
}

/**
 * @brief  Verifies that the SPI bus is not busy before starting a transaction.
 *         Waits up to SPI_TIMEOUT cycles for the BSY flag to clear.
 * @retval SPI_OK    Bus is free and ready.
 * @retval SPI_BUSY  Bus could not be acquired within timeout.
 */
static SPI_Status_t prv_check_bus(void)
{
    if (!(SPI1->SR & SPI_SR_BSY)) return SPI_OK;

    uint32_t t = SPI_TIMEOUT;
    while ((SPI1->SR & SPI_SR_BSY) && --t) { }

    if (t == 0U) return SPI_BUSY;

    return SPI_OK;
}

/**
 * @brief  Transmits one byte and returns the simultaneously received byte.
 *         Waits for TXE before loading DR, then waits for RXNE to read DR.
 *         This ensures full-duplex synchronization per RM0383.
 * @param  byte  Byte to transmit.
 * @param  rx    Pointer to store the received byte. If NULL, byte is discarded.
 * @retval SPI_OK   Byte exchange completed successfully.
 * @retval SPI_ERR  Timeout or hardware error during transfer.
 */
static SPI_Status_t prv_transfer_byte(uint8_t byte, uint8_t *rx)
{
    /* Wait until TX buffer is empty */
    if (!prv_wait_sr(SPI_SR_TXE))  return SPI_ERR;

    /* Load the byte into the data register — starts the clock */
    *(__IO uint8_t *)&SPI1->DR = byte;

    /* Wait until RX buffer is not empty (byte fully shifted in) */
    if (!prv_wait_sr(SPI_SR_RXNE)) return SPI_ERR;

    /* Read the received byte — clears RXNE */
    uint8_t received = *(__IO uint8_t *)&SPI1->DR;
    if (rx != NULL) *rx = received;

    /* Check for hardware errors after the transfer */
    if (prv_has_error()) return SPI_ERR;

    return SPI_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  INITIALIZATION                                                             */
/* ══════════════════════════════════════════════════════════════════════════ */

void spi_init(void)
{
    /* Enable RCC clocks for GPIOA and SPI1; dummy reads ensure clock is active
     * SPI1 is on APB2 (high-speed bus) */
    RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    /* ── Configure SPI GPIO pins ────────────────────────────────────────── */
    GPIO_PinCfg_t cfg_af;
    cfg_af.mode  = GPIO_MODE_ALT_FN;
    cfg_af.otype = GPIO_OTYPE_PUSH_PULL;
    cfg_af.speed = GPIO_SPEED_HIGH;
    cfg_af.pull  = GPIO_PULL_NONE;

    /* PA5 = SCK, PA6 = MISO, PA7 = MOSI — alternate function 5 */
    gpio_initPort(SPI_SCK_PORT);
    gpio_setPinMode(SPI_SCK_PORT,  SPI_SCK_PIN,  &cfg_af);
    gpio_setAlternateFunction(SPI_SCK_PORT,  SPI_SCK_PIN,  SPI_AF);
    gpio_setPinMode(SPI_MISO_PORT, SPI_MISO_PIN, &cfg_af);
    gpio_setAlternateFunction(SPI_MISO_PORT, SPI_MISO_PIN, SPI_AF);
    gpio_setPinMode(SPI_MOSI_PORT, SPI_MOSI_PIN, &cfg_af);
    gpio_setAlternateFunction(SPI_MOSI_PORT, SPI_MOSI_PIN, SPI_AF);

    /* PA4 = CS — GPIO output push-pull, starts HIGH (device deselected) */
    GPIO_PinCfg_t cfg_cs;
    cfg_cs.mode  = GPIO_MODE_OUTPUT;
    cfg_cs.otype = GPIO_OTYPE_PUSH_PULL;
    cfg_cs.speed = GPIO_SPEED_HIGH;
    cfg_cs.pull  = GPIO_PULL_NONE;

    gpio_setPinMode(SPI_CS_PORT, SPI_CS_PIN, &cfg_cs);
    gpio_setPin(SPI_CS_PORT, SPI_CS_PIN);           /* CS HIGH = deselected */

    /* ── Configure SPI1 peripheral ─────────────────────────────────────── */
    /* Disable SPI before configuring registers (RM0383 requirement) */
    SPI1->CR1 = 0U;
    SPI1->CR2 = 0U;

    /*
     * CR1 configuration:
     *   BIDIMODE = 0  → Full-duplex (2-line unidirectional)
     *   CRCEN    = 0  → CRC disabled
     *   DFF      = 0  → 8-bit data frame format
     *   RXONLY   = 0  → Full-duplex (transmit and receive)
     *   SSM      = 1  → Software slave management enabled
     *   SSI      = 1  → Internal slave select HIGH (master not MODF'd)
     *   LSBFIRST = 0  → MSB transmitted first
     *   BR[2:0]  = 010 → fPCLK/8 ≈ 2 MHz (SPI1 on APB2 at 16 MHz)
     *   MSTR     = 1  → Master mode
     *   CPOL     = 0  → Clock idle LOW  (Mode 0)
     *   CPHA     = 0  → Data captured on first (rising) edge (Mode 0)
     */
    SPI1->CR1 = SPI_CR1_SSM      |   /* Software slave management           */
                SPI_CR1_SSI      |   /* Internal slave select (master mode)  */
                SPI_CR1_MSTR     |   /* Master mode                          */
                SPI_CR1_BR_DIV8;     /* Baud rate: fPCLK/8                  */

    /* CR2: no DMA, no interrupt, SSOE disabled (we drive CS manually) */
    SPI1->CR2 = 0U;

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  CHIP SELECT CONTROL                                                        */
/* ══════════════════════════════════════════════════════════════════════════ */

void spi_csEnable(void)
{
    /* Drive CS LOW to select the target device */
    gpio_clearPin(SPI_CS_PORT, SPI_CS_PIN);
}

void spi_csDisable(void)
{
    /*
     * Wait for BSY to clear before releasing CS.
     * The BSY flag is cleared when the last bit has been fully shifted out,
     * ensuring the slave has latched the complete byte before CS goes HIGH.
     */
    uint32_t t = SPI_TIMEOUT;
    while ((SPI1->SR & SPI_SR_BSY) && --t) { }

    /* Drive CS HIGH to deselect the device */
    gpio_setPin(SPI_CS_PORT, SPI_CS_PIN);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  TRANSMIT                                                                   */
/* ══════════════════════════════════════════════════════════════════════════ */

SPI_Status_t spi_transmit(const uint8_t *data, uint32_t len)
{
    /* Validate arguments: non-null buffer and non-zero length */
    if (data == NULL || len == 0U) return SPI_ERR;

    /* Ensure the bus is free before starting */
    if (prv_check_bus() != SPI_OK) return SPI_BUSY;

    for (uint32_t i = 0U; i < len; i++)
    {
        /* Send each byte; received byte is discarded (NULL rx pointer) */
        if (prv_transfer_byte(data[i], NULL) != SPI_OK) return SPI_ERR;
    }

    return SPI_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  RECEIVE                                                                    */
/* ══════════════════════════════════════════════════════════════════════════ */

SPI_Status_t spi_receive(uint8_t *data, uint32_t len)
{
    /* Validate arguments: non-null buffer and non-zero length */
    if (data == NULL || len == 0U) return SPI_ERR;

    /* Ensure the bus is free before starting */
    if (prv_check_bus() != SPI_OK) return SPI_BUSY;

    for (uint32_t i = 0U; i < len; i++)
    {
        /*
         * Drive the clock by sending a dummy byte (0x00).
         * The slave shifts out its data on MISO while we send the dummy byte.
         * prv_transfer_byte() captures the incoming byte into data[i].
         */
        if (prv_transfer_byte(0x00U, &data[i]) != SPI_OK) return SPI_ERR;
    }

    return SPI_OK;
}