/*
 * stm32_i2c_io.c
 *
 * STM32F103C8 backend for the i2c_io.h abstraction on I2C1 (PB6 = SCL,
 * PB7 = SDA).
 *
 * Uses open-drain / pull-up GPIO waveform generation on PB6/PB7 so that:
 * 1. It works identically on real STM32F103 silicon and in the Wokwi
 *    simulator (bypassing Wokwi STM32 hardware I2C1 Bug #926 where
 *    hardware I2C1 ACKs the final read byte and locks SDA low).
 * 2. Every I2C transaction produces deterministic START, 8-bit data,
 *    9th-clock ACK/NACK, and STOP waveforms on PB6 (SCL) and PB7 (SDA)
 *    for LCD1602 (0x27), DS1307 RTC (0x68), and 24C256 EEPROM (0x50).
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "os.h"
#include "i2c_io.h"
#include "dio.h"
#include "define.h"
#include "stm32f1xx.h"

#define I2C_MAX_BUS_NUMBER  (1u)

/* PB6 = SCL, PB7 = SDA (Port B = 2 in dio.h) */
static dio_t s_sclPin = { .port = 2U, .pin = 6U };
static dio_t s_sdaPin = { .port = 2U, .pin = 7U };

static inline void i2c_delay(void)
{
    dio_delay_us(4U);
}

/* Open-drain emulation:
 * - HIGH (release line): INPUT_PULLUP enables internal pull-up so line
 *   rises to 3.3V while allowing slaves to pull SDA low for ACK/data.
 * - LOW (drive low): OUTPUT with do_reset() pulls line to 0V. */
static inline void scl_high(void)
{
    dio_mode(&s_sclPin, INPUT_PULLUP);
}

static inline void scl_low(void)
{
    do_reset(&s_sclPin);
    dio_mode(&s_sclPin, OUTPUT);
    do_reset(&s_sclPin);
}

static inline void sda_high(void)
{
    dio_mode(&s_sdaPin, INPUT_PULLUP);
}

static inline void sda_low(void)
{
    do_reset(&s_sdaPin);
    dio_mode(&s_sdaPin, OUTPUT);
    do_reset(&s_sdaPin);
}

static inline bool sda_read(void)
{
    return di_read(&s_sdaPin);
}

static void i2c_bb_start(void)
{
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

static void i2c_bb_stop(void)
{
    scl_low();
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

/* Returns true if slave ACKed (pulled SDA low on 9th clock) */
static bool i2c_bb_write_byte(uint8_t byte)
{
    for (uint8_t bit = 0U; bit < 8U; bit++) {
        if ((byte & 0x80U) != 0U) {
            sda_high();
        } else {
            sda_low();
        }
        byte <<= 1U;
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        i2c_delay();
    }

    /* 9th clock: release SDA and check slave ACK */
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    bool ack = !sda_read();
    scl_low();
    i2c_delay();

    return ack;
}

static uint8_t i2c_bb_read_byte(bool send_ack)
{
    uint8_t byte = 0U;

    sda_high();
    for (uint8_t bit = 0U; bit < 8U; bit++) {
        byte <<= 1U;
        i2c_delay();
        scl_high();
        i2c_delay();
        if (sda_read()) {
            byte |= 1U;
        }
        scl_low();
        i2c_delay();
    }

    /* 9th clock: drive SDA low for ACK, or leave high for NACK on final byte */
    if (send_ack) {
        sda_low();
    } else {
        sda_high();
    }
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();
    sda_high();

    return byte;
}

uint32_t i2c_io_init(i2c_io_t *me)
{
    if ((me == NULL) || (me->ui32I2cPort == 0U) || (me->ui32I2cPort > I2C_MAX_BUS_NUMBER)) {
        return ERROR_INVALID_PARAM;
    }

    uint32_t err = i2c_io_lock_init(me);
    if (err != ERROR_OK) {
        return err;
    }

    /* Ensure hardware I2C1 is disabled so PB6/PB7 are controlled cleanly by GPIO */
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;

    scl_high();
    sda_high();
    i2c_delay();

    return ERROR_OK;
}

uint32_t i2c_io_set_speed(i2c_io_t *me, uint32_t ui32SpeedHz)
{
    if ((me == NULL) || (ui32SpeedHz == 0U) || (ui32SpeedHz > 400000U)) {
        return ERROR_INVALID_PARAM;
    }

    return ERROR_OK;
}

uint32_t i2c_io_write_sync(i2c_io_t *me, uint16_t ui16DevAddr,
                           const uint8_t *pui8TxBuff, uint32_t ui32Length)
{
    if ((me == NULL) || (pui8TxBuff == NULL) || (ui32Length == 0U)) {
        return ERROR_INVALID_PARAM;
    }

    i2c_bb_start();

    if (!i2c_bb_write_byte((uint8_t)((ui16DevAddr << 1U) | 0U))) {
        i2c_bb_stop();
        return ERROR_NACK;
    }

    for (uint32_t i = 0U; i < ui32Length; i++) {
        if (!i2c_bb_write_byte(pui8TxBuff[i])) {
            i2c_bb_stop();
            return ERROR_NACK;
        }
    }

    i2c_bb_stop();
    return ERROR_OK;
}

uint32_t i2c_io_read_sync(i2c_io_t *me, uint16_t ui16DevAddr,
                          uint8_t *pui8RxBuff, uint32_t ui32Length)
{
    if ((me == NULL) || (pui8RxBuff == NULL) || (ui32Length == 0U)) {
        return ERROR_INVALID_PARAM;
    }

    i2c_bb_start();

    if (!i2c_bb_write_byte((uint8_t)((ui16DevAddr << 1U) | 1U))) {
        i2c_bb_stop();
        return ERROR_NACK;
    }

    for (uint32_t i = 0U; i < ui32Length; i++) {
        bool send_ack = (i < (ui32Length - 1U));
        pui8RxBuff[i] = i2c_bb_read_byte(send_ack);
    }

    i2c_bb_stop();
    return ERROR_OK;
}

uint32_t i2c_io_write_read_sync(i2c_io_t *me, uint16_t ui16DevAddr,
                                const uint8_t *pui8TxBuff, uint32_t ui32TxLength,
                                uint8_t *pui8RxBuff, uint32_t ui32RxLength)
{
    if ((me == NULL) || (pui8TxBuff == NULL) || (ui32TxLength == 0U) ||
        (pui8RxBuff == NULL) || (ui32RxLength == 0U)) {
        return ERROR_INVALID_PARAM;
    }

    uint32_t err = i2c_io_write_sync(me, ui16DevAddr, pui8TxBuff, ui32TxLength);
    if (err != ERROR_OK) {
        return err;
    }

    return i2c_io_read_sync(me, ui16DevAddr, pui8RxBuff, ui32RxLength);
}
