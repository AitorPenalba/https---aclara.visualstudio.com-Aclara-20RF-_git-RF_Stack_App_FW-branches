/* generated common source file - do not edit */
#include "common_data.h"
elc_instance_ctrl_t g_elc_ctrl;

extern const elc_cfg_t g_elc_cfg;

const elc_instance_t g_elc = {
    .p_ctrl = &g_elc_ctrl,
    .p_api  = &g_elc_on_elc,
    .p_cfg  = &g_elc_cfg
};
icu_instance_ctrl_t miso_busy_ctrl;
const external_irq_cfg_t miso_busy_cfg =
{
    // TODO: RA6E1 Bob: Once the whole team is converted to Rev B equivalent hardware, this can be simplified
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A )    /* Modification to generated file by Aclara */
    #warning "You have built project EP_FreeRTOS_RA6 for Y84580 Rev A (P1A) hardware"
    .channel             = 4,                               /* Modification to generated file by Aclara */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B )  /* Modification to generated file by Aclara */
    #warning "You have built project EP_FreeRTOS_RA6 for Y84580 Rev B (P1B) hardware"
    .channel             = 14,                              /* Modification to generated file by Aclara */
#else                                                       /* Modification to generated file by Aclara */
   #error "Invalid value for HAL_TARGET_HARDWARE"           /* Modification to generated file by Aclara */
#endif                                                      /* Modification to generated file by Aclara */
    .trigger             = EXTERNAL_IRQ_TRIG_RISING,
    .filter_enable       = false,
    .pclk_div            = EXTERNAL_IRQ_PCLK_DIV_BY_64,
    .p_callback          = isr_busy,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = NULL,
    .ipl                 = (12),
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A )    /* Modification to generated file by Aclara */
  #if defined(VECTOR_NUMBER_ICU_IRQ4)                       /* Modification to generated file by Aclara */
    .irq                 = VECTOR_NUMBER_ICU_IRQ4,          /* Modification to generated file by Aclara */
  #else                                                     /* Modification to generated file by Aclara */
    .irq                 = FSP_INVALID_VECTOR,              /* Modification to generated file by Aclara */
  #endif                                                    /* Modification to generated file by Aclara */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B )  /* Modification to generated file by Aclara */
  #if defined(VECTOR_NUMBER_ICU_IRQ14)                      /* Modification to generated file by Aclara */
    .irq                 = VECTOR_NUMBER_ICU_IRQ14,         /* Modification to generated file by Aclara */
  #else                                                     /* Modification to generated file by Aclara */
    .irq                 = FSP_INVALID_VECTOR,              /* Modification to generated file by Aclara */
  #endif                                                    /* Modification to generated file by Aclara */
#endif                                                      /* Modification to generated file by Aclara */
};
/* Instance structure to use this module. */
const external_irq_instance_t miso_busy =
{
    .p_ctrl        = &miso_busy_ctrl,
    .p_cfg         = &miso_busy_cfg,
    .p_api         = &g_external_irq_on_icu
};
icu_instance_ctrl_t hmc_trouble_busy_ctrl;
const external_irq_cfg_t hmc_trouble_busy_cfg =
{
    // TODO: RA6E1 Bob: Once the whole team is converted to Rev B equivalent hardware, this can be simplified
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A )    /* Modification to generated file by Aclara */
    .channel             = 14,                              /* Modification to generated file by Aclara */
    #warning "You have built project EP_FreeRTOS_RA6 for Y84580 Rev A (P1A) hardware"
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B )  /* Modification to generated file by Aclara */
    #warning "You have built project EP_FreeRTOS_RA6 for Y84580 Rev B (P1B) hardware"
    .channel             =  4,                              /* Modification to generated file by Aclara */
#else                                                       /* Modification to generated file by Aclara */
   #error "Invalid value for HAL_TARGET_HARDWARE"           /* Modification to generated file by Aclara */
#endif                                                      /* Modification to generated file by Aclara */
    .trigger             = EXTERNAL_IRQ_TRIG_BOTH_EDGE,
    .filter_enable       = false,
    .pclk_div            = EXTERNAL_IRQ_PCLK_DIV_BY_64,
    .p_callback          = meter_trouble_isr_busy,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = NULL,
    .ipl                 = (12),
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A )    /* Modification to generated file by Aclara */
  #if defined(VECTOR_NUMBER_ICU_IRQ14)                      /* Modification to generated file by Aclara */
    .irq                 = VECTOR_NUMBER_ICU_IRQ14,         /* Modification to generated file by Aclara */
  #else                                                     /* Modification to generated file by Aclara */
    .irq                 = FSP_INVALID_VECTOR,              /* Modification to generated file by Aclara */
  #endif                                                    /* Modification to generated file by Aclara */
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B )  /* Modification to generated file by Aclara */
  #if defined(VECTOR_NUMBER_ICU_IRQ4)                       /* Modification to generated file by Aclara */
    .irq                 = VECTOR_NUMBER_ICU_IRQ4,          /* Modification to generated file by Aclara */
  #else                                                     /* Modification to generated file by Aclara */
    .irq                 = FSP_INVALID_VECTOR,              /* Modification to generated file by Aclara */
  #endif                                                    /* Modification to generated file by Aclara */
#endif
};
/* Instance structure to use this module. */
const external_irq_instance_t hmc_trouble_busy =
{
    .p_ctrl        = &hmc_trouble_busy_ctrl,
    .p_cfg         = &hmc_trouble_busy_cfg,
    .p_api         = &g_external_irq_on_icu
};
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
icu_instance_ctrl_t g_external_irq0_ctrl;
const external_irq_cfg_t g_external_irq0_cfg =
{
    .channel             = 13,
    .trigger             = EXTERNAL_IRQ_TRIG_FALLING,
    .filter_enable       = false,
    .pclk_div            = EXTERNAL_IRQ_PCLK_DIV_BY_64,
    .p_callback          = Radio0_IRQ_ISR,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = NULL,
    .ipl                 = (12),
#if defined(VECTOR_NUMBER_ICU_IRQ13)
    .irq                 = VECTOR_NUMBER_ICU_IRQ13,
#else
    .irq                 = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const external_irq_instance_t g_external_irq0 =
{
    .p_ctrl        = &g_external_irq0_ctrl,
    .p_cfg         = &g_external_irq0_cfg,
    .p_api         = &g_external_irq_on_icu
};
icu_instance_ctrl_t pf_meter_ctrl;
const external_irq_cfg_t pf_meter_cfg =
{
    .channel             = 11,
    .trigger             = EXTERNAL_IRQ_TRIG_BOTH_EDGE,
    .filter_enable       = true,
    .pclk_div            = EXTERNAL_IRQ_PCLK_DIV_BY_64,
    .p_callback          = isr_brownOut,
    /** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
    .p_context           = &NULL,
#endif
    .p_extend            = NULL,
    .ipl                 = (1),
#if defined(VECTOR_NUMBER_ICU_IRQ11)
    .irq                 = VECTOR_NUMBER_ICU_IRQ11,
#else
    .irq                 = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const external_irq_instance_t pf_meter =
{
    .p_ctrl        = &pf_meter_ctrl,
    .p_cfg         = &pf_meter_cfg,
    .p_api         = &g_external_irq_on_icu
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
