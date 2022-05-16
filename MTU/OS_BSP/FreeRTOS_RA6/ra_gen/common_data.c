/* generated common source file - do not edit */
#include "common_data.h"
const cgc_cfg_t g_cgc0_cfg =
{
    .p_callback = NULL,
};
cgc_instance_ctrl_t g_cgc0_ctrl;
const cgc_instance_t g_cgc0 = {
    .p_api = &g_cgc_on_cgc,
    .p_ctrl        = &g_cgc0_ctrl,
    .p_cfg         = &g_cgc0_cfg,
};
elc_instance_ctrl_t g_elc_ctrl;

extern const elc_cfg_t g_elc_cfg;

const elc_instance_t g_elc = {
    .p_ctrl = &g_elc_ctrl,
    .p_api  = &g_elc_on_elc,
    .p_cfg  = &g_elc_cfg
};
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
        {
            .p_api = &g_ioport_on_ioport,
            .p_ctrl = &g_ioport_ctrl,
            .p_cfg = &g_bsp_pin_cfg,
        };
void g_common_init(void) {
}
