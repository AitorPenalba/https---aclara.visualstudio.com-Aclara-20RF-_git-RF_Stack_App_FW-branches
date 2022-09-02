/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_qspi.h"
            #include "r_spi_flash_api.h"
#include "r_iwdt.h"
            #include "r_wdt_api.h"
#include "r_flash_hp.h"
#include "r_flash_api.h"
FSP_HEADER
extern const spi_flash_instance_t g_qspi0;
            extern qspi_instance_ctrl_t g_qspi0_ctrl;
            extern const spi_flash_cfg_t g_qspi0_cfg;
/** WDT on IWDT Instance. */
extern const wdt_instance_t g_wdt0;


/** Access the IWDT instance using these structures when calling API functions directly (::p_api is not used). */
extern iwdt_instance_ctrl_t g_wdt0_ctrl;
extern const wdt_cfg_t g_wdt0_cfg;

#ifndef NULL
void NULL(wdt_callback_args_t * p_args);
#endif
/* Flash on Flash HP Instance */
extern const flash_instance_t g_flash0;

/** Access the Flash HP instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_hp_instance_ctrl_t g_flash0_ctrl;
extern const flash_cfg_t g_flash0_cfg;

#ifndef NULL
void NULL(flash_callback_args_t * p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
