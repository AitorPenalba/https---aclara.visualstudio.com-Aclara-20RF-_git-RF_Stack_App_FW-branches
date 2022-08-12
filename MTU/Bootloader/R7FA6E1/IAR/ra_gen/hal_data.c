/* generated HAL source file - do not edit */
#include "hal_data.h"
iwdt_instance_ctrl_t g_wdt0_ctrl;

const wdt_cfg_t g_wdt0_cfg =
{
    .p_callback = NULL,
};

/* Instance structure to use this module. */
const wdt_instance_t g_wdt0 =
{
    .p_ctrl        = &g_wdt0_ctrl,
    .p_cfg         = &g_wdt0_cfg,
    .p_api         = &g_wdt_on_iwdt
};
void g_hal_init(void) {
g_common_init();
}
