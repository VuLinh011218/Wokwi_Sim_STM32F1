/*
 * lcd1602_i2c.c
 */

#include <stddef.h>
#include <stdbool.h>

#include "lcd1602_i2c.h"
#include "i2c_io.h"
#include "dio.h"
#include "define.h"

/* PCF8574 bit positions */
#define LCD_BIT_RS      (0x01u)
#define LCD_BIT_RW      (0x02u)
#define LCD_BIT_EN      (0x04u)
#define LCD_BIT_BL      (0x08u)

/* HD44780 instructions */
#define LCD_CMD_CLEAR_DISPLAY       (0x01u)
#define LCD_CMD_RETURN_HOME         (0x02u)
#define LCD_CMD_ENTRY_MODE_SET      (0x04u)
#define LCD_CMD_DISPLAY_CONTROL     (0x08u)
#define LCD_CMD_FUNCTION_SET        (0x20u)
#define LCD_CMD_SET_DDRAM_ADDR      (0x80u)

#define LCD_ENTRY_LEFT_TO_RIGHT     (0x02u)
#define LCD_DISPLAY_ON              (0x04u)
#define LCD_FUNCTION_4BIT_2LINE     (0x08u)

static const uint8_t lcd_row_offset[LCD1602_ROWS] = { 0x00U, 0x40U };

static uint32_t lcd1602_i2c_write(lcd1602_t *me, uint8_t ui8Byte)
{
    return i2c_io_write_sync(me->pI2cBus, me->ui16DevAddr, &ui8Byte, 1U);
}

static uint32_t lcd1602_write_nibble(lcd1602_t *me, uint8_t ui8Nibble, bool bRs)
{
    uint8_t base = ui8Nibble | (bRs ? LCD_BIT_RS : 0U) |
                   (me->bBacklightOn ? LCD_BIT_BL : 0U);

    uint32_t err = lcd1602_i2c_write(me, base | LCD_BIT_EN);

    if (err == ERROR_OK) {
        dio_delay_us(2U);
        err = lcd1602_i2c_write(me, base);
        dio_delay_us(40U);
    }

    return err;
}

static uint32_t lcd1602_send_byte(lcd1602_t *me, uint8_t ui8Byte, bool bRs)
{
    uint32_t err = i2c_io_lock(me->pI2cBus, I2C_WAIT_TIME);

    if (err != ERROR_OK) {
        return err;
    }

    err = lcd1602_write_nibble(me, ui8Byte & 0xF0U, bRs);

    if (err == ERROR_OK) {
        err = lcd1602_write_nibble(me, (uint8_t)((ui8Byte << 4U) & 0xF0U), bRs);
    }

    i2c_io_unlock(me->pI2cBus);

    return err;
}

uint32_t lcd1602_command(lcd1602_t *me, uint8_t ui8Cmd)
{
    if (me == NULL) {
        return ERROR_INVALID_PARAM;
    }

    uint32_t err = lcd1602_send_byte(me, ui8Cmd, false);
    if ((ui8Cmd == LCD_CMD_CLEAR_DISPLAY) || (ui8Cmd == LCD_CMD_RETURN_HOME)) {
        dio_delay_us(2000U);
    }
    return err;
}

uint32_t lcd1602_data(lcd1602_t *me, uint8_t ui8Data)
{
    if (me == NULL) {
        return ERROR_INVALID_PARAM;
    }

    return lcd1602_send_byte(me, ui8Data, true);
}

uint32_t lcd1602_init(lcd1602_t *me, const lcd1602_init_t *init)
{
    if ((me == NULL) || (init == NULL) || (init->pI2cBus == NULL)) {
        return ERROR_INVALID_PARAM;
    }

    me->pI2cBus = init->pI2cBus;
    me->ui16DevAddr = init->ui16DevAddr;
    me->bBacklightOn = true;

    dio_delay_us(15000U);

    uint32_t err = i2c_io_lock(me->pI2cBus, I2C_WAIT_TIME);
    if (err != ERROR_OK) {
        return err;
    }

    err = lcd1602_write_nibble(me, 0x30U, false);
    dio_delay_us(4500U);

    if (err == ERROR_OK) {
        err = lcd1602_write_nibble(me, 0x30U, false);
        dio_delay_us(4500U);
    }

    if (err == ERROR_OK) {
        err = lcd1602_write_nibble(me, 0x30U, false);
        dio_delay_us(150U);
    }

    if (err == ERROR_OK) {
        err = lcd1602_write_nibble(me, 0x20U, false); /* switch to 4-bit */
        dio_delay_us(150U);
    }

    i2c_io_unlock(me->pI2cBus);

    if (err == ERROR_OK) {
        err = lcd1602_command(me, LCD_CMD_FUNCTION_SET | LCD_FUNCTION_4BIT_2LINE);
    }

    if (err == ERROR_OK) {
        err = lcd1602_command(me, LCD_CMD_DISPLAY_CONTROL | LCD_DISPLAY_ON);
    }

    if (err == ERROR_OK) {
        err = lcd1602_clear(me);
    }

    if (err == ERROR_OK) {
        err = lcd1602_command(me, LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_LEFT_TO_RIGHT);
    }

    return err;
}

uint32_t lcd1602_clear(lcd1602_t *me)
{
    return lcd1602_command(me, LCD_CMD_CLEAR_DISPLAY);
}

uint32_t lcd1602_home(lcd1602_t *me)
{
    return lcd1602_command(me, LCD_CMD_RETURN_HOME);
}

uint32_t lcd1602_set_cursor(lcd1602_t *me, uint8_t ui8Col, uint8_t ui8Row)
{
    if ((me == NULL) || (ui8Row >= LCD1602_ROWS) || (ui8Col >= LCD1602_COLS)) {
        return ERROR_INVALID_PARAM;
    }

    return lcd1602_command(me, LCD_CMD_SET_DDRAM_ADDR | (lcd_row_offset[ui8Row] + ui8Col));
}

uint32_t lcd1602_write_char(lcd1602_t *me, char c)
{
    return lcd1602_data(me, (uint8_t)c);
}

uint32_t lcd1602_print(lcd1602_t *me, const char *pcStr)
{
    if ((me == NULL) || (pcStr == NULL)) {
        return ERROR_INVALID_PARAM;
    }

    uint32_t err = ERROR_OK;

    while ((*pcStr != '\0') && (err == ERROR_OK)) {
        err = lcd1602_write_char(me, *pcStr);
        pcStr++;
    }

    return err;
}

uint32_t lcd1602_backlight(lcd1602_t *me, bool bOn)
{
    if (me == NULL) {
        return ERROR_INVALID_PARAM;
    }

    me->bBacklightOn = bOn;

    uint8_t byte = bOn ? LCD_BIT_BL : 0x00U;

    uint32_t err = i2c_io_lock(me->pI2cBus, I2C_WAIT_TIME);

    if (err != ERROR_OK) {
        return err;
    }

    err = i2c_io_write_sync(me->pI2cBus, me->ui16DevAddr, &byte, 1U);

    i2c_io_unlock(me->pI2cBus);

    return err;
}

uint32_t lcd1602_display_on_off(lcd1602_t *me, bool bOn)
{
    if (me == NULL) {
        return ERROR_INVALID_PARAM;
    }

    return lcd1602_command(me, LCD_CMD_DISPLAY_CONTROL | (bOn ? LCD_DISPLAY_ON : 0U));
}
