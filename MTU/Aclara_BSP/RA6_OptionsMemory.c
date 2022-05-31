/***********************************************************************************************************************
 * Copyright [2020-2021] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics America Inc. and may only be used with products
 * of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.  Renesas products are
 * sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for the selection and use
 * of Renesas products and Renesas assumes no liability.  No license, express or implied, to any intellectual property
 * right is granted by Renesas. This software is protected under all applicable laws, including copyright laws. Renesas
 * reserves the right to change or discontinue this software and/or this documentation. THE SOFTWARE AND DOCUMENTATION
 * IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT
 * PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE OR
 * DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.  TO THE MAXIMUM
 * EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR DOCUMENTATION
 * (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING,
 * WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY LOST PROFITS,
 * OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#include "bsp_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define RA_NOT_DEFINED    (0)

/** OR in the HOCO frequency setting from bsp_clock_cfg.h with the OFS1 setting from bsp_cfg.h. */
#define BSP_ROM_REG_OFS1_SETTING                                             \
    (((uint32_t) BSP_CFG_ROM_REG_OFS1 & BSP_FEATURE_BSP_OFS1_HOCOFRQ_MASK) | \
     ((uint32_t) BSP_CFG_HOCO_FREQUENCY << BSP_FEATURE_BSP_OFS1_HOCOFRQ_OFFSET))

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/

       /* OFS0 Option Memory */
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_ofs0") g_bsp_rom_ofs0 =
    BSP_CFG_ROM_REG_OFS0;

       /* DUALSEL Option Memory */
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_dualsel") g_bsp_rom_dualsel =
    BSP_CFG_ROM_REG_DUALSEL;

BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_sas") g_bsp_rom_sas =
    0xFFFFFFFF;

       /* OFS1 Option Memory */
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_ofs1") g_bsp_rom_ofs1 =
    BSP_ROM_REG_OFS1_SETTING;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_ofs1_sec") g_bsp_rom_ofs1_sec =
    BSP_ROM_REG_OFS1_SETTING;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_ofs1_sel") g_bsp_rom_ofs1_sel =
    BSP_CFG_ROM_REG_OFS1_SEL;

       /* BANKSEL Option Memory */
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_banksel") g_bsp_rom_banksel =
    0xFFFFFFFF;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_banksel_sec") g_bsp_rom_banksel_sec =
    0xFFFFFFFF;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_banksel_sel") g_bsp_rom_banksel_sel =
    0xFFFFFFFF;

       /* BPS and PBPS Option Memory */
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps0") g_bsp_rom_bps0 =
    BSP_CFG_ROM_REG_BPS0;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps1") g_bsp_rom_bps1 =
    BSP_CFG_ROM_REG_BPS1;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps2") g_bsp_rom_bps2 =
    BSP_CFG_ROM_REG_BPS2;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sec0") g_bsp_rom_bps_sec0 =
    BSP_CFG_ROM_REG_BPS0;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sec1") g_bsp_rom_bps_sec1 =
    BSP_CFG_ROM_REG_BPS1;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sec2") g_bsp_rom_bps_sec2 =
    BSP_CFG_ROM_REG_BPS2;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_pbps0") g_bsp_rom_pbps0 =
    BSP_CFG_ROM_REG_PBPS0;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_pbps_sec0") g_bsp_rom_pbps_sec0 =
    BSP_CFG_ROM_REG_PBPS0;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_pbps_sec1") g_bsp_rom_pbps_sec1 =
    BSP_CFG_ROM_REG_PBPS1;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_pbps_sec2") g_bsp_rom_pbps_sec2 =
    BSP_CFG_ROM_REG_PBPS2;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sel0") g_bsp_rom_bps_sel0 =
    BSP_CFG_ROM_REG_BPS_SEL0;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sel1") g_bsp_rom_bps_sel1 =
    BSP_CFG_ROM_REG_BPS_SEL1;
BSP_DONT_REMOVE static const uint32_t BSP_PLACE_IN_SECTION(".option_setting_bps_sel2") g_bsp_rom_bps_sel2 =
    BSP_CFG_ROM_REG_BPS_SEL2;
