/*
 * dht22.c
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "dht22.h"
#include "dio.h"
#include "define.h"

#define DHT_START_LOW_US         (2000u)  /* >=1ms for DHT22, >=18ms for physical DHT11 */
#define DHT_START_LOW_DHT11_US   (18000u)
#define DHT_RESPONSE_TIMEOUT_US  (200u)
#define DHT_BIT_TIMEOUT_US       (150u)
#define DHT_BIT_ONE_THRESHOLD_US (45u)    /* ~26-28us = '0', ~70us = '1' */

uint32_t dht22_init(dht22_t *me, const dht22_init_t *pInit)
{
    if ((me == NULL) || (pInit == NULL) || (pInit->pDataPin == NULL)) {
        return ERROR_INVALID_PARAM;
    }

    me->pDataPin = pInit->pDataPin;
    me->eType = pInit->eType;

    dio_mode(me->pDataPin, INPUT_PULLUP);

    return ERROR_OK;
}

uint32_t dht22_read(dht22_t *me, dht22_reading_t *pOut)
{
    if ((me == NULL) || (me->pDataPin == NULL) || (pOut == NULL)) {
        return ERROR_INVALID_PARAM;
    }

    uint8_t pui8Data[5] = { 0U, 0U, 0U, 0U, 0U };
    uint32_t ui32StartLowUs = (me->eType == DHT_TYPE_DHT11) ?
                              DHT_START_LOW_DHT11_US : DHT_START_LOW_US;

    /* 1. Host start signal: pull LOW, then release to INPUT_PULLUP */
    dio_mode(me->pDataPin, OUTPUT);
    do_reset(me->pDataPin);
    dio_delay_us(ui32StartLowUs);

    do_set(me->pDataPin);
    dio_delay_us(30U);
    dio_mode(me->pDataPin, INPUT_PULLUP);

    /* 2. Wait for sensor response:
     *    - Remaining high idle before sensor pulls low
     *    - ~80us LOW response pulse
     *    - ~80us HIGH response pulse */
    if (dio_measure_pulse_us(me->pDataPin, true, DHT_RESPONSE_TIMEOUT_US) == 0xFFFFFFFFUL) {
        return ERROR_TIMEOUT;
    }

    if (dio_measure_pulse_us(me->pDataPin, false, DHT_RESPONSE_TIMEOUT_US) == 0xFFFFFFFFUL) {
        return ERROR_TIMEOUT;
    }

    if (dio_measure_pulse_us(me->pDataPin, true, DHT_RESPONSE_TIMEOUT_US) == 0xFFFFFFFFUL) {
        return ERROR_TIMEOUT;
    }

    /* 3. Read 40 bits (5 bytes, MSB first):
     *    Each bit starts with ~50us LOW followed by 26-28us HIGH ('0') or ~70us HIGH ('1'). */
    for (uint8_t ui8Bit = 0U; ui8Bit < 40U; ui8Bit++) {
        if (dio_measure_pulse_us(me->pDataPin, false, DHT_BIT_TIMEOUT_US) == 0xFFFFFFFFUL) {
            return ERROR_TIMEOUT;
        }

        uint32_t ui32HighUs = dio_measure_pulse_us(me->pDataPin, true, DHT_BIT_TIMEOUT_US);

        if (ui32HighUs == 0xFFFFFFFFUL) {
            return ERROR_TIMEOUT;
        }

        pui8Data[ui8Bit / 8U] <<= 1U;

        if (ui32HighUs > DHT_BIT_ONE_THRESHOLD_US) {
            pui8Data[ui8Bit / 8U] |= 1U;
        }
    }

    /* 4. Verify 8-bit checksum */
    uint8_t ui8Sum = (uint8_t)(pui8Data[0] + pui8Data[1] + pui8Data[2] + pui8Data[3]);

    if (ui8Sum != pui8Data[4]) {
        return ERROR_FAIL;
    }

    /* 5. Decode according to sensor type */
    if (me->eType == DHT_TYPE_DHT11) {
        pOut->ui16HumidityTenthsPct = (uint16_t)pui8Data[0] * 10U + (uint16_t)(pui8Data[1] % 10U);
        int16_t i16Temp = (int16_t)pui8Data[2] * 10 + (int16_t)(pui8Data[3] % 10U);
        if ((pui8Data[3] & 0x80U) != 0U) {
            i16Temp = -i16Temp;
        }
        pOut->i16TempTenthsC = i16Temp;
    } else {
        pOut->ui16HumidityTenthsPct = (uint16_t)(((uint16_t)pui8Data[0] << 8U) | pui8Data[1]);

        uint16_t ui16RawTemp = (uint16_t)(((uint16_t)pui8Data[2] << 8U) | pui8Data[3]);
        int16_t i16Temp = (int16_t)(ui16RawTemp & 0x7FFFU);

        if ((ui16RawTemp & 0x8000U) != 0U) {
            i16Temp = -i16Temp;
        }

        pOut->i16TempTenthsC = i16Temp;
    }

    return ERROR_OK;
}
