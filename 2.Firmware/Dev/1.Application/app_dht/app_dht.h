/*
 * app_dht.h
 *
 * Periodically samples the DHT temperature/humidity sensor via bsp_dht
 * (every 2000ms - see g_pTaskTable in app_main.c) and caches the latest
 * valid reading for other application modules (app_system for LCD
 * rendering, app_log for EEPROM snapshot logging).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "dht22.h"

void app_dht_init(void);

/* Scheduler task - runs every 2000ms (DHT22 minimum sampling period). */
void app_dht_task(void);

/* Copies the last valid reading into *pReading. Returns false if no
 * valid reading has been captured yet. */
bool app_dht_get_reading(dht22_reading_t *pReading);

/* Sequence-tracked consumer API (same pattern as app_rtc_consume_if_updated). */
bool app_dht_consume_if_updated(dht22_reading_t *pReading, uint32_t *pui32LastSeenSeq);
