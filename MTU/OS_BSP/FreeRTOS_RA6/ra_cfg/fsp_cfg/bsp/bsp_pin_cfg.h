/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"
#define PF_METER (BSP_IO_PORT_00_PIN_06) /* PF_Meter */
#define UART4_RX_DBG (BSP_IO_PORT_02_PIN_06) /* UART4_RX_DBG */
#define UART4_TX_DBG (BSP_IO_PORT_02_PIN_07) /* UART4_TX_DBG */
#define UART3_RX_MFG (BSP_IO_PORT_04_PIN_08) /* UART3_RX_MSP */
#define UART3_TX_MFG (BSP_IO_PORT_04_PIN_09) /* UART3_TX_MSP */
extern const ioport_cfg_t g_bsp_pin_cfg; /* I-210+c_RA6 */
extern const ioport_cfg_t g_bsp_pin_cfg_lpm; /* I-210+c_RA6_LPM */

void BSP_PinConfigSecurityInit();
#endif /* BSP_PIN_CFG_H_ */
