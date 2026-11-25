/*
 * app_log.c
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_log.h"
#include "bsp_eeprom.h"
#include "app_rtc.h"
#include "app_dht.h"
#include "ds1307.h"
#include "dht22.h"
#include "define.h"

#define APP_LOG_MAX_RECORDS  (500u)
#define APP_LOG_HEADER_ADDR  (0x0000u)
#define APP_LOG_RECORDS_ADDR (APP_LOG_HEADER_ADDR + (uint16_t)sizeof(app_log_header_t))

static const uint8_t s_pui8Magic[4] = { 'D', 'L', 'O', 'G' };

typedef struct __attribute__((packed))
{
    uint8_t  pui8Magic[4];
    uint32_t ui32NextIndex;
}
app_log_header_t; /* 8 bytes */

static app_log_header_t s_header;

void app_log_init(void)
{
    memset(&s_header, 0, sizeof(s_header));

    if (bsp_eeprom_read(APP_LOG_HEADER_ADDR, (uint8_t *)&s_header, sizeof(s_header)) != ERROR_OK ||
        memcmp(s_header.pui8Magic, s_pui8Magic, sizeof(s_pui8Magic)) != 0) {
        memcpy(s_header.pui8Magic, s_pui8Magic, sizeof(s_pui8Magic));
        s_header.ui32NextIndex = 0U;
        bsp_eeprom_write(APP_LOG_HEADER_ADDR, (const uint8_t *)&s_header, sizeof(s_header));
    }
}

bool app_log_record_snapshot(void)
{
    app_log_record_t rec;
    memset(&rec, 0, sizeof(rec));

    ds1307_time_t time;
    if (app_rtc_get_time(&time)) {
        rec.ui8Seconds = time.ui8Seconds;
        rec.ui8Minutes = time.ui8Minutes;
        rec.ui8Hours   = time.ui8Hours;
        rec.ui8Date    = time.ui8Date;
        rec.ui8Month   = time.ui8Month;
        rec.ui16Year   = time.ui16Year;
    }

    dht22_reading_t dht;
    if (app_dht_get_reading(&dht)) {
        rec.i16TempTenthsC        = dht.i16TempTenthsC;
        rec.ui16HumidityTenthsPct = dht.ui16HumidityTenthsPct;
    }

    uint32_t ui32Slot = s_header.ui32NextIndex % APP_LOG_MAX_RECORDS;
    uint16_t ui16Addr = (uint16_t)(APP_LOG_RECORDS_ADDR + (ui32Slot * sizeof(app_log_record_t)));

    if (bsp_eeprom_write(ui16Addr, (const uint8_t *)&rec, sizeof(rec)) != ERROR_OK) {
        return false;
    }

    s_header.ui32NextIndex++;

    uint16_t ui16IdxAddr = (uint16_t)(APP_LOG_HEADER_ADDR + offsetof(app_log_header_t, ui32NextIndex));
    return (bsp_eeprom_write(ui16IdxAddr,
                             (const uint8_t *)&s_header.ui32NextIndex,
                             sizeof(s_header.ui32NextIndex)) == ERROR_OK);
}

bool app_log_get_last_record(app_log_record_t *pOutRecord, uint32_t *pui32OutCount)
{
    if ((pOutRecord == NULL) || (s_header.ui32NextIndex == 0U)) {
        return false;
    }

    uint32_t ui32LastSlot = (s_header.ui32NextIndex - 1U) % APP_LOG_MAX_RECORDS;
    uint16_t ui16Addr = (uint16_t)(APP_LOG_RECORDS_ADDR + (ui32LastSlot * sizeof(app_log_record_t)));

    if (bsp_eeprom_read(ui16Addr, (uint8_t *)pOutRecord, sizeof(app_log_record_t)) != ERROR_OK) {
        return false;
    }

    if (pui32OutCount != NULL) {
        *pui32OutCount = s_header.ui32NextIndex;
    }

    return true;
}
