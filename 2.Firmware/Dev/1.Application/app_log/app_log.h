/*
 * app_log.h
 *
 * Ring-buffer logger in the 24C256 I2C EEPROM (0x50) via bsp_eeprom.
 * Stores timestamped temperature/humidity snapshots whenever the user
 * presses the button, and reads back the latest snapshot from EEPROM
 * for display on the LCD.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed))
{
    uint8_t  ui8Seconds;
    uint8_t  ui8Minutes;
    uint8_t  ui8Hours;
    uint8_t  ui8Date;
    uint8_t  ui8Month;
    uint16_t ui16Year;
    int16_t  i16TempTenthsC;
    uint16_t ui16HumidityTenthsPct;
}
app_log_record_t; /* 11 bytes */

void app_log_init(void);

/* Captures current RTC time + DHT reading and writes a new record to
 * EEPROM. Returns true if the EEPROM write succeeded. */
bool app_log_record_snapshot(void);

/* Reads the most recently written record directly back from EEPROM into
 * *pOutRecord, and writes the total record count into *pui32OutCount.
 * Returns false if no records have been saved yet. */
bool app_log_get_last_record(app_log_record_t *pOutRecord, uint32_t *pui32OutCount);
