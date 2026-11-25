/*
 * bsp_dht.c
 */

#include "bsp_dht.h"
#include "dht22.h"
#include "dio.h"

#define BSP_DHT_GPIO_PORT_A  (1u)  /* 1 = GPIOA in dio.h */
#define BSP_DHT_GPIO_PIN_0   (0u)  /* PA0 */

static dio_t s_dhtPin = { .port = BSP_DHT_GPIO_PORT_A, .pin = BSP_DHT_GPIO_PIN_0 };
static dht22_t s_dht;

void bsp_dht_init(void)
{
    dht22_init_t init = {
        .pDataPin = &s_dhtPin,
        .eType = DHT_TYPE_DHT22 /* Wokwi uses wokwi-dht22; change to DHT_TYPE_DHT11 for physical DHT11 */
    };

    dht22_init(&s_dht, &init);
}

uint32_t bsp_dht_read(dht22_reading_t *pReading)
{
    return dht22_read(&s_dht, pReading);
}
