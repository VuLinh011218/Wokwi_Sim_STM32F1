/*
 * eeprom-24c256.chip.c
 *
 * Wokwi Custom Chip simulating an AT24C256 I2C EEPROM (32KB = 32768 bytes)
 * at 7-bit I2C address 0x50.
 *
 * Protocol supported (matching Dev/3.Drivers/Devices/eeprom24cxx):
 * - Write transaction: [addr_hi, addr_lo, data_0, data_1, ...]
 *   - 1-byte write [0x00] (used by eeprom24cxx_wait_write_done() ACK-polling)
 *     is safely ACKed without modifying the current address pointer or memory.
 *   - 2-byte write [addr_hi, addr_lo] sets the internal 15-bit address pointer
 *     (used before a Repeated-START read in i2c_io_write_read_sync()).
 *   - >=3-byte write [addr_hi, addr_lo, data...] writes each data byte into
 *     memory[addr] and increments addr modulo 32768.
 * - Read transaction: returns memory[addr] and increments addr modulo 32768.
 */

#include "wokwi-api.h"
#include <stdint.h>
#include <stdbool.h>

#define EEPROM_I2C_ADDR   (0x50u)
#define EEPROM_SIZE_BYTES (32768u)

typedef struct
{
    uint16_t addr;
    uint16_t pending_addr_hi;
    uint8_t  write_byte_index;
    uint8_t  mem[EEPROM_SIZE_BYTES];
}
eeprom_chip_t;

static eeprom_chip_t s_chip;

static bool on_i2c_connect(void *user_data, uint32_t address, bool read)
{
    eeprom_chip_t *chip = (eeprom_chip_t *)user_data;
    (void)address;

    if (!read) {
        chip->write_byte_index = 0U;
    }

    return true; /* ACK */
}

static uint8_t on_i2c_read(void *user_data)
{
    eeprom_chip_t *chip = (eeprom_chip_t *)user_data;
    uint8_t data = chip->mem[chip->addr & (EEPROM_SIZE_BYTES - 1U)];
    chip->addr = (uint16_t)((chip->addr + 1U) & (EEPROM_SIZE_BYTES - 1U));
    return data;
}

static bool on_i2c_write(void *user_data, uint8_t data)
{
    eeprom_chip_t *chip = (eeprom_chip_t *)user_data;

    if (chip->write_byte_index == 0U) {
        /* Hold MSB temporarily so a 1-byte ACK poll doesn't corrupt chip->addr */
        chip->pending_addr_hi = (uint16_t)data;
        chip->write_byte_index = 1U;
    } else if (chip->write_byte_index == 1U) {
        chip->addr = (uint16_t)(((chip->pending_addr_hi << 8U) | (uint16_t)data) & (EEPROM_SIZE_BYTES - 1U));
        chip->write_byte_index = 2U;
    } else {
        chip->mem[chip->addr & (EEPROM_SIZE_BYTES - 1U)] = data;
        chip->addr = (uint16_t)((chip->addr + 1U) & (EEPROM_SIZE_BYTES - 1U));
    }

    return true; /* ACK */
}

static void on_i2c_disconnect(void *user_data)
{
    (void)user_data;
}

void chip_init(void)
{
    for (uint32_t i = 0U; i < EEPROM_SIZE_BYTES; i++) {
        s_chip.mem[i] = 0xFFU;
    }
    s_chip.addr = 0U;
    s_chip.pending_addr_hi = 0U;
    s_chip.write_byte_index = 0U;

    const i2c_config_t i2c_cfg = {
        .user_data  = &s_chip,
        .address    = EEPROM_I2C_ADDR,
        .scl        = pin_init("SCL", INPUT_PULLUP),
        .sda        = pin_init("SDA", INPUT_PULLUP),
        .connect    = on_i2c_connect,
        .read       = on_i2c_read,
        .write      = on_i2c_write,
        .disconnect = on_i2c_disconnect,
    };

    i2c_init(&i2c_cfg);
}
