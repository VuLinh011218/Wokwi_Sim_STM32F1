/*
 * app_dht.c
 */

#include <stddef.h>
#include <stdbool.h>

#include "app_dht.h"
#include "bsp_dht.h"
#include "define.h"

static dht22_reading_t s_latestReading;
static uint32_t s_ui32UpdateSeq = 0U;
static bool s_bHasReading = false;

void app_dht_init(void)
{
    bsp_dht_init();
}

void app_dht_task(void)
{
    dht22_reading_t reading;

    if (bsp_dht_read(&reading) == ERROR_OK) {
        s_latestReading = reading;
        s_ui32UpdateSeq++;
        s_bHasReading = true;
    }
}

bool app_dht_get_reading(dht22_reading_t *pReading)
{
    if ((pReading == NULL) || !s_bHasReading) {
        return false;
    }

    *pReading = s_latestReading;
    return true;
}

bool app_dht_consume_if_updated(dht22_reading_t *pReading, uint32_t *pui32LastSeenSeq)
{
    if ((pReading == NULL) || (pui32LastSeenSeq == NULL) || !s_bHasReading) {
        return false;
    }

    if (*pui32LastSeenSeq == s_ui32UpdateSeq) {
        return false;
    }

    *pReading = s_latestReading;
    *pui32LastSeenSeq = s_ui32UpdateSeq;
    return true;
}
