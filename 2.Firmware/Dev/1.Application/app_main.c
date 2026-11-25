/*
 * app_main.c
 *
 * Cooperative scheduler task table + interrupt event table for the
 * STM32F103C8T6 Wokwi Demo (Button, LED, LCD1602 I2C, RTC DS1307,
 * EEPROM 24C256, DHT Temperature/Humidity sensor).
 */

#include <stdint.h>
#include <stdbool.h>

#include "app_main.h"
#include "scheduler.h"
#include "event.h"
#include "app_rtc.h"
#include "app_dht.h"
#include "app_log.h"
#include "app_system.h"
#include "app_exit_button.h"
#include "bsp_exit_button.h"
#include "app_lcd.h"
#include "app_led.h"

tSchedulerTask g_pTaskTable[5] =
{
    {
        &app_rtc_task,
        1000,   /* poll DS1307 RTC every 1000ms (1s) */
        0,
        true
    },
    {
        &app_dht_task,
        2000,   /* sample DHT sensor every 2000ms (2s) */
        0,
        true
    },
    {
        &app_system_task,
        100,    /* update system state machine & LCD buffer every 100ms */
        0,
        true
    },
    {
        &app_lcd_task,
        100,    /* push dirty LCD buffer over I2C1 to LCD1602 every 100ms */
        0,
        true
    },
    {
        &app_led_task,
        APP_LED_BLINK_PERIOD_MS / 2U, /* toggle PC13 LED every 500ms (1Hz heartbeat) */
        0,
        true
    }
};
uint32_t g_ui32SchedulerNumTasks = 5U;

tEventEntry g_pEventTable[1] = {
    [BSP_EXIT_BUTTON_EVENT_PRESSED] = { &app_exit_button_on_press, false }
};
uint32_t g_ui32EventNumEvents = 1U;

void app_main(void)
{
    scheduler_init(1000); /* 1000 ticks/sec -> 1ms tick */

    /* Initialize LED and LCD first so visual feedback appears immediately */
    app_led_init();
    app_system_init();
    app_lcd_init();
    app_lcd_task();

    app_rtc_init();
    app_dht_init();
    app_log_init();
    app_exit_button_init();

    /* Perform an initial sensor + RTC read and update the LCD right away */
    app_rtc_task();
    app_dht_task();
    app_system_task();
    app_lcd_task();

    for (;;) {
        scheduler_run();
        event_run();
    }
}
