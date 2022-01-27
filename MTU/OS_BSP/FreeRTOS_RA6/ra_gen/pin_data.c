/* generated pin source file - do not edit */
#include "bsp_api.h"
#include "r_ioport_api.h"


const ioport_pin_cfg_t g_bsp_pin_cfg_data[] = {
    {
        .pin = BSP_IO_PORT_00_PIN_00,
        .pin_cfg = ((uint32_t) IOPORT_CFG_ANALOG_ENABLE)
    },
    {
        .pin = BSP_IO_PORT_00_PIN_03,
        .pin_cfg = ((uint32_t) IOPORT_CFG_ANALOG_ENABLE)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_00,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_QSPI)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_01,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_QSPI)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_02,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_QSPI)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_03,
        .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SPI)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_08,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_DEBUG)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_09,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_DEBUG)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_10,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_DEBUG)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_12,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_QSPI)
    },
    {
        .pin = BSP_IO_PORT_01_PIN_14,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_GPT1)
    },
    {
        .pin = BSP_IO_PORT_03_PIN_00,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_DEBUG)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_00,
        .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_IIC)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_01,
        .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_IIC)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_07,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_CLKOUT_COMP_RTC)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_10,
        .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SPI)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_11,
        .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SPI)
    },
    {
        .pin = BSP_IO_PORT_04_PIN_12,
        .pin_cfg = ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SPI)
    },
};

const ioport_cfg_t g_bsp_pin_cfg = {
    .number_of_pins = sizeof(g_bsp_pin_cfg_data)/sizeof(ioport_pin_cfg_t),
    .p_pin_cfg_data = &g_bsp_pin_cfg_data[0],
};

#if BSP_TZ_SECURE_BUILD

void R_BSP_PinCfgSecurityInit(void);

/* Initialize SAR registers for secure pins. */
void R_BSP_PinCfgSecurityInit(void)
{
    uint16_t pmsar[BSP_FEATURE_BSP_NUM_PMSAR];
    memset(pmsar, 0xFF, BSP_FEATURE_BSP_NUM_PMSAR * sizeof(R_PMISC->PMSAR[0]));


    for(uint32_t i = 0; i < g_bsp_pin_cfg.number_of_pins; i++)
    {
        uint32_t port_pin = g_bsp_pin_cfg.p_pin_cfg_data[i].pin;
        uint32_t port = port_pin >> 8U;
        uint32_t pin = port_pin & 0xFFU;
        pmsar[port] &= (uint16_t) ~(1U << pin);
    }

    for(uint32_t i = 0; i < BSP_FEATURE_BSP_NUM_PMSAR; i++)
    {
        R_PMISC->PMSAR[i].PMSAR = pmsar[i];
    }

}
#endif
