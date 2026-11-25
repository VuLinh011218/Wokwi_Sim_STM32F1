/*
 * app_system.c
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_system.h"
#include "app_rtc.h"
#include "app_dht.h"
#include "app_log.h"
#include "os.h"

#define APP_SYSTEM_POPUP_DURATION_MS  (3000u)

typedef enum
{
    APP_SYS_STATE_LIVE = 0,
    APP_SYS_STATE_SHOW_SAVED
}
app_sys_state_t;

static app_sys_state_t s_eState = APP_SYS_STATE_LIVE;
static uint32_t s_ui32PopupStartTick = 0U;

static uint32_t s_ui32LastRtcSeq = 0U;
static uint32_t s_ui32LastDhtSeq = 0U;

static ds1307_time_t s_cachedTime;
static bool s_bHasTime = false;

static dht22_reading_t s_cachedDht;
static bool s_bHasDht = false;

static app_system_display_t s_display;
static uint32_t s_ui32DisplaySeq = 0U;

static void app_system_set_display(const char *pszLine1, const char *pszLine2)
{
    if ((strncmp(s_display.szLine1, pszLine1, APP_SYSTEM_DISPLAY_LINE_LEN - 1U) == 0) &&
        (strncmp(s_display.szLine2, pszLine2, APP_SYSTEM_DISPLAY_LINE_LEN - 1U) == 0)) {
        return;
    }

    strncpy(s_display.szLine1, pszLine1, APP_SYSTEM_DISPLAY_LINE_LEN - 1U);
    s_display.szLine1[APP_SYSTEM_DISPLAY_LINE_LEN - 1U] = '\0';

    strncpy(s_display.szLine2, pszLine2, APP_SYSTEM_DISPLAY_LINE_LEN - 1U);
    s_display.szLine2[APP_SYSTEM_DISPLAY_LINE_LEN - 1U] = '\0';

    s_ui32DisplaySeq++;
}

static void app_system_render_live(void)
{
    char szL1[APP_SYSTEM_DISPLAY_LINE_LEN];
    char szL2[APP_SYSTEM_DISPLAY_LINE_LEN];

    if (s_bHasTime) {
        snprintf(szL1, sizeof(szL1), "%02u:%02u:%02u %02u/%02u",
                 (unsigned)s_cachedTime.ui8Hours,
                 (unsigned)s_cachedTime.ui8Minutes,
                 (unsigned)s_cachedTime.ui8Seconds,
                 (unsigned)s_cachedTime.ui8Date,
                 (unsigned)s_cachedTime.ui8Month);
    } else {
        snprintf(szL1, sizeof(szL1), "RTC: waiting...");
    }

    if (s_bHasDht) {
        int16_t i16T = s_cachedDht.i16TempTenthsC;
        char cSign = (i16T < 0) ? '-' : '+';
        uint16_t ui16AbsT = (uint16_t)((i16T < 0) ? -i16T : i16T);
        uint16_t ui16H = s_cachedDht.ui16HumidityTenthsPct;

        snprintf(szL2, sizeof(szL2), "T:%c%02u.%uC H:%02u.%u%%",
                 cSign,
                 (unsigned)(ui16AbsT / 10U),
                 (unsigned)(ui16AbsT % 10U),
                 (unsigned)(ui16H / 10U),
                 (unsigned)(ui16H % 10U));
    } else {
        snprintf(szL2, sizeof(szL2), "DHT: reading...");
    }

    app_system_set_display(szL1, szL2);
}

void app_system_init(void)
{
    s_eState = APP_SYS_STATE_LIVE;
    app_system_set_display("STM32 Wokwi Demo", "Init sensors...");
}

void app_system_task(void)
{
    ds1307_time_t newTime;
    if (app_rtc_consume_if_updated(&newTime, &s_ui32LastRtcSeq)) {
        s_cachedTime = newTime;
        s_bHasTime = true;
    }

    dht22_reading_t newDht;
    if (app_dht_consume_if_updated(&newDht, &s_ui32LastDhtSeq)) {
        s_cachedDht = newDht;
        s_bHasDht = true;
    }

    if (s_eState == APP_SYS_STATE_SHOW_SAVED) {
        if ((os_tick_get() - s_ui32PopupStartTick) >= APP_SYSTEM_POPUP_DURATION_MS) {
            s_eState = APP_SYS_STATE_LIVE;
            app_system_render_live();
        }
        return;
    }

    app_system_render_live();
}

void app_system_notify_button_pressed(bool bEepromOk)
{
    char szL1[APP_SYSTEM_DISPLAY_LINE_LEN];
    char szL2[APP_SYSTEM_DISPLAY_LINE_LEN];
    app_log_record_t rec;
    uint32_t ui32Count = 0U;

    s_eState = APP_SYS_STATE_SHOW_SAVED;
    s_ui32PopupStartTick = os_tick_get();

    if (bEepromOk && app_log_get_last_record(&rec, &ui32Count)) {
        int16_t i16T = rec.i16TempTenthsC;
        uint16_t ui16AbsT = (uint16_t)((i16T < 0) ? -i16T : i16T);

        snprintf(szL1, sizeof(szL1), "EEPROM SAVED #%lu", (unsigned long)ui32Count);
        snprintf(szL2, sizeof(szL2), "%02u:%02u %02u.%uC %02u%%",
                 (unsigned)rec.ui8Hours,
                 (unsigned)rec.ui8Minutes,
                 (unsigned)(ui16AbsT / 10U),
                 (unsigned)(ui16AbsT % 10U),
                 (unsigned)(rec.ui16HumidityTenthsPct / 10U));
    } else {
        snprintf(szL1, sizeof(szL1), "EEPROM WRITE ERR");
        snprintf(szL2, sizeof(szL2), "Check 24C256 I2C");
    }

    app_system_set_display(szL1, szL2);
}

bool app_system_consume_display_if_updated(app_system_display_t *pOut, uint32_t *pui32LastSeenSeq)
{
    if ((pOut == NULL) || (pui32LastSeenSeq == NULL) || (s_ui32DisplaySeq == 0U)) {
        return false;
    }

    if (*pui32LastSeenSeq == s_ui32DisplaySeq) {
        return false;
    }

    *pOut = s_display;
    *pui32LastSeenSeq = s_ui32DisplaySeq;
    return true;
}
