/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other restrictions.
*****************************************************************************
*
* Comments:
*
*   User configuration for MQX components
*
*
*END************************************************************************/

#ifndef __user_config_h__
#define __user_config_h__

/* mandatory CPU identification */
#define MQX_CPU                 PSP_CPU_MK24F120M


/* BSP Configuration */

/*
** The clock tick rate in miliseconds - be cautious to keep this value such
** that it divides 1000 well
**
** MGCT: <option type="number" min="1" max="1000"/>
*/
#define BSP_ALARM_FREQUENCY             (200)

/* BSP settings - for defaults see mqx\source\bsp\ep_k24f120m\ep_k24f120m.h */
#define BSPCFG_ENABLE_CFMPROTECT                  0   // Bootloader handles this capability

#define BSPCFG_ENABLE_TTYA                        0 // Manufacturing serial port
#define BSPCFG_ENABLE_ITTYA                       1
#define BSPCFG_SCI0_BAUD_RATE                     38400
#define BSPCFG_SCI0_QUEUE_SIZE                    1024

#define BSPCFG_ENABLE_TTYB                        0 // Optical pass through port
#define BSPCFG_ENABLE_ITTYB                       1
#define BSPCFG_SCI1_BAUD_RATE                     9600
#define BSPCFG_SCI1_QUEUE_SIZE                    256

#define BSPCFG_ENABLE_TTYC                        0 // Debug port
#define BSPCFG_ENABLE_ITTYC                       1
#define BSPCFG_SCI2_BAUD_RATE                     115200
#define BSPCFG_SCI2_QUEUE_SIZE                    1024

#define BSPCFG_ENABLE_TTYD                        0 // Meter port
#define BSPCFG_ENABLE_ITTYD                       1
#define BSPCFG_SCI3_BAUD_RATE                     9600
#define BSPCFG_SCI3_QUEUE_SIZE                    128

#define BSPCFG_ENABLE_TTYE                        0 // Host port
#define BSPCFG_ENABLE_ITTYE                       1
#define BSPCFG_SCI4_BAUD_RATE                     115200
#define BSPCFG_SCI4_QUEUE_SIZE                    2048 // Max supported HDLC frame size

#define BSP_DEFAULT_IO_CHANNEL                    "ittyc:" // Debug port
#define BSP_DEFAULT_IO_CHANNEL_DEFINED

/* BSP settings - for defaults see mqx\source\bsp\<board_name>\<board_name>.h */
#define BSPCFG_ENABLE_I2C0                        0   /* Turn off, MQX default is on */
#define BSPCFG_ENABLE_II2C0                       1

#define BSPCFG_ENABLE_SPI0                        0 // Serial flash - using Aclara driver instead of mqx
#define BSPCFG_ENABLE_SPI1                        0 // Silabs radio - using Aclara driver instead of mqx
#define BSPCFG_ENABLE_RTCDEV                      1

/* Aclara Added */
#define RTC_CR_SCxP_DEFAULT                       ( RTC_CR_SC2P_MASK | RTC_CR_SC16P_MASK )

#define BSPCFG_ENABLE_ADC0                        1
#define BSPCFG_ENABLE_ADC1                        1
#define BSPCFG_ENABLE_LWADC0                      1
#define BSPCFG_ENABLE_LWADC1                      1

#define BSPCFG_ENABLE_IODEBUG                     0

/* Choose clock source for USB module */
#define BSPCFG_USB_USE_IRC48M                     0 /* Do not use internal reference clock source */

#define BSPCFG_HAS_SRAM_POOL                      0
#define BSPCFG_ENET_SRAM_BUF                      0

/* board-specific MQX settings - see for defaults mqx\source\include\mqx_cnfg.h */
#define MQX_USE_IDLE_TASK                         1
#define MQX_ENABLE_LOW_POWER                      0
#define MQX_ENABLE_HSRUN                          0   /* USB can not work if MQX_ENABLE_HSRUN equal to 0 */
#define MQX_USE_TIMER                             1
#define MQXCFG_ENABLE_FP                          1
#define MQX_INCLUDE_FLOATING_POINT_IO             0
#define MQX_KERNEL_LOGGING                        1   // Needed to print tasks stack usage
#define MQX_USE_LOGS                              0
#define MQX_SAVE_FP_ALWAYS                        1   // Needed to save the FP contex inside an ISR

/*
** include common settings
*/


/* use the rest of defaults from small-RAM-device profile */
#include "small_ram_config.h"

/* and enable verification checks in kernel */
#include "verif_enabled_config.h"

#endif /* __user_config_h__ */
