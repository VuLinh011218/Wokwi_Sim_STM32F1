/*
 * dht22.h
 *
 * 1-Wire temperature & relative-humidity sensor driver supporting both
 * DHT22 (AM2302 - Wokwi's built-in "wokwi-dht22" part) and DHT11.
 *
 * No board knowledge (receives its data pin through dht22_init_t) and
 * no MCU-register knowledge (uses Dev/3.Drivers/Components/dio's
 * dio_mode/do_set/do_reset/dio_delay_us/dio_measure_pulse_us).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "dio.h"

typedef enum
{
    DHT_TYPE_DHT22 = 0, /* 16-bit big-endian tenths of %RH and tenths of degC */
    DHT_TYPE_DHT11 = 1  /* 8-bit integer + 8-bit decimal bytes */
}
dht22_type_t;

typedef struct
{
    dio_t        *pDataPin;
    dht22_type_t  eType;
}
dht22_init_t;

typedef struct
{
    dio_t        *pDataPin;
    dht22_type_t  eType;
}
dht22_t;

typedef struct
{
    int16_t  i16TempTenthsC;        /* e.g. 254 -> 25.4 C, -52 -> -5.2 C */
    uint16_t ui16HumidityTenthsPct; /* e.g. 605 -> 60.5 %RH */
}
dht22_reading_t;

uint32_t dht22_init(dht22_t *me, const dht22_init_t *pInit);
uint32_t dht22_read(dht22_t *me, dht22_reading_t *pOut);
