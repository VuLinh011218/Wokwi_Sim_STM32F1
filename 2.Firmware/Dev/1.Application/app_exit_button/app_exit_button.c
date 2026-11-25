/*
 * app_exit_button.c
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_exit_button.h"
#include "bsp_exit_button.h"
#include "app_system.h"
#include "app_log.h"
#include "os.h"

#define APP_EXIT_BUTTON_DEBOUNCE_MS  (200u)

static uint32_t s_ui32LastAcceptedTick = 0U;
static bool s_bHasAcceptedBefore = false;

void app_exit_button_init(void)
{
    bsp_exit_button_init();
}

void app_exit_button_on_press(void)
{
    uint32_t ui32Now = os_tick_get();

    if (s_bHasAcceptedBefore && ((ui32Now - s_ui32LastAcceptedTick) < APP_EXIT_BUTTON_DEBOUNCE_MS)) {
        return; /* ignore contact bounce within 200ms */
    }

    s_ui32LastAcceptedTick = ui32Now;
    s_bHasAcceptedBefore = true;

    bool bSaved = app_log_record_snapshot();
    app_system_notify_button_pressed(bSaved);
}
