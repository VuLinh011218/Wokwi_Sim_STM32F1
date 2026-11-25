/*
 * app_system.h
 *
 * Central orchestrator for the STM32F103C8T6 Wokwi Demo:
 * - Combines cached RTC time (app_rtc) and DHT sensor data (app_dht)
 *   to format the 16x2 LCD lines in live-monitoring mode.
 * - When notified of a button press (app_system_notify_button_pressed),
 *   switches the LCD to display the snapshot just written & read back
 *   from the 24C256 EEPROM (app_log) for 3 seconds before returning to
 *   the live screen.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define APP_SYSTEM_DISPLAY_LINE_LEN  (17u)  /* 16 visible chars + NUL */

typedef struct
{
    char szLine1[APP_SYSTEM_DISPLAY_LINE_LEN];
    char szLine2[APP_SYSTEM_DISPLAY_LINE_LEN];
}
app_system_display_t;

void app_system_init(void);

/* Scheduler task - runs every 100ms. */
void app_system_task(void);

/* Called by app_exit_button_on_press() after recording a snapshot to EEPROM. */
void app_system_notify_button_pressed(bool bEepromOk);

/* Consumed by app_lcd_task() to repaint the 16x2 LCD only when changed. */
bool app_system_consume_display_if_updated(app_system_display_t *pOut, uint32_t *pui32LastSeenSeq);
