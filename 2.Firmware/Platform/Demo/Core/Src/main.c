/*
 * main.c
 *
 * Minimal STM32F103C8T6 startup entry point that hands control directly
 * to Dev/1.Application/app_main.c (cooperative scheduler + event loop).
 */

#include "main.h"
#include "app_main.h"

void SystemClock_Config(void);

int main(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

    /* Free PA15/PB3/PB4 from JTAG so all GPIO pins behave as standard GPIO */
    LL_GPIO_AF_DisableRemap_SWJ();

    SystemClock_Config();

    /* Hand off to the 5-layer application & cooperative scheduler */
    app_main();

    while (1) {
    }
}

void SystemClock_Config(void)
{
    /* Default reset clock on STM32F103 is 8 MHz HSI (AHB/APB1/APB2 div = 1).
     * Avoid unbounded polling on RCC_CR_HSIRDY / FLASH_ACR_LATENCY so the
     * startup sequence never hangs on register-level simulators like Wokwi. */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
    LL_RCC_HSI_Enable();
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

    LL_SetSystemCoreClock(8000000U);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}
