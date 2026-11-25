/*
 * bsp_dht.h
 *
 * Binds the DHT temperature/humidity sensor driver (dht22) to PA0 on
 * the STM32F103C8T6 Blue Pill board (matching diagram.json's dht1:SDA ->
 * bluepill:A0 connection).
 */

#pragma once

#include <stdint.h>

#include "dht22.h"

void bsp_dht_init(void);
uint32_t bsp_dht_read(dht22_reading_t *pReading);
