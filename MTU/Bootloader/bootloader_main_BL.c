/***********************************************************************************************************************
 *
 * Filename:   bootloader_main.c
 *
 * Global Designator: BL_MAIN_
 *
 * Contents: Contains the firmware version definitions
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* Include Files */
#include "project_BL.h"

#include "partitions_BL.h"
#include "partition_cfg_BL.h"
#include "dvr_intFlash_cfg_BL.h"
#include "IF_intFlash_BL.h"
#include "BSP_aclara_BL.h"
#include "crc32.h"


/*lint -esym(526, app_vector, __set_SP) defined at time of link */
/*lint -esym(628, __set_SP) no protoype available */

/* ****************************************************************************************************************** */
/* Macro Definitions */
#define MAX_COPY_RANGES          2        /* Maximum number of ranges to copy from NV to ROM.   */
#define BOOTLOADER_UT_ENABLE     0        /* Enabled the bootloader unit test code */
#define DEBOUNCE_CHECK_LIMIT     10       /* number of iterations to check PF */
#define DEBOUNCE_CHECK_MS_DELAY  10       /* ms delay between successive checks of PF */
#define SYSTICK_CYCLES_1MS       120000   /* number of cycles in 1ms */
#define LED_INDICATOR_DELAY_MS   5000     /* ms delay to display LED's for each bootloader test case */
#define SYST_RVR_VALUE           600000   /* reaload value for SYS_RVR register */
#define IRC48MCLK                0        /* Enabled 48MIRC clock instad of the 8MHz crystal clock */

#if BOOTLOADER_UT_ENABLE
#warning "Disable BOOTLOADER_UT_ENABLE before releasing"
#define BOOTLOADER_TEST(x) BootloaderUnitTest(x)
#else
#define BOOTLOADER_TEST(x)
#endif //endif BOOTLOADER_UT_ENABLE

#if ( MCU_SELECTED == NXP_K24 )

/******************************************************************************
 * Register settings for FBE Mode
 ******************************************************************************/
#if ( IRC48MCLK == 1 )
#define MCG_C1_IN_FBE     0xBEU     /* MCGOUTCLK Src=ExtRefClk, FLLExtRefDiv=3(31250Hz=48MHz/1536), FLLIRCSel=Slow,
                                      IRCEn=MCGIRCLK active, IntRefStopEn=True */
#define MCG_C2_IN_FBE     0x94U    /* LossOfClockResetEnable=reset, FastIRCFineTrim=None, RANGE=High Freq,
                                      OscGain=LowPwr, ExtRef=ExtOsc, LowPowerSel=FLL/PLL not disabled in bypass modes,
                                      IRCSel=Slow */
#define MCG_C4_IN_FBE     0x00U    /* Unchanged from FEI (default): FLL Factor=640, DCO=lowRange, FCT and SCF Trims
                                      cleared */
#define MCG_C5_IN_FBE     0x00U    /* Unchanged from FEI (default): PLL disabled, PLL disabled in all Stop modes, PLL
                                      SrcDiv=1 */
#define MCG_C6_IN_FBE     0x00U    /* Loss of Lock Interrupt disabled, FLL selected, Clock Monitor Disabled, VCO 0
                                      Divider=24 */
#define MCG_C7_IN_FBE     0x02U    /* OSCSEL = 2 to select OSCCLK1 */
#define OSC_CR_IN_FBE     0x80U    /* Ext Ref clock Enabled, Ext Ref clock disabled in Stop mode, Osc Load Caps=0pF */
#else
#define MCG_C1_IN_FBE     0x9EU    /* MCGOUTCLK Src=ExtRefClk, FLLExtRefDiv=3(31250Hz=8MHz/256), FLLIRCSel=Slow,
                                      IRCEn=MCGIRCLK active, IntRefStopEn=True */

#define MCG_C2_IN_FBE     0x94U    /* LossOfClockResetEnable=reset, FastIRCFineTrim=None, RANGE=High Freq,
                                      OscGain=LowPwr, ExtRef=ExtOsc, LowPowerSel=FLL/PLL not disabled in bypass modes,
                                      IRCSel=Slow */
#define MCG_C4_IN_FBE     0x00U    /* Unchanged from FEI (default): FLL Factor=640, DCO=lowRange, FCT and SCF Trims
                                      cleared */
#define MCG_C5_IN_FBE     0x00U    /* Unchanged from FEI (default): PLL disabled, PLL disabled in all Stop modes, PLL
                                      SrcDiv=1 */
#define MCG_C6_IN_FBE     0x20U    /* Loss of Lock Interrupt disabled, FLL selected, Clock Monitor Enabled, VCO 0
                                      Divider=24 */
#define MCG_C7_IN_FBE     0x00U    /* OSCSEL = 0 to select OSCCLK0 */
#define OSC_CR_IN_FBE     0x80U    /* Ext Ref clock Enabled, Ext Ref clock disabled in Stop mode, Osc Load Caps=0pF */
#endif

/******************************************************************************
 * Register settings for PBE Mode
 ******************************************************************************/
#if ( IRC48MCLK == 1 )
#define MCG_C1_IN_PBE     0xBEU    /* Unchanged from FBE mode */
#define MCG_C2_IN_PBE     0x94U    /* Unchanged from FBE mode */
#define MCG_C5_IN_PBE     0x4BU    /* PLL Clock Enabled, PLL disabled in all Stop modes, PLL SrcDiv=12(48MHz->4MHz) */
#define MCG_C6_IN_PBE     0x46U    /* Loss of Lock Interrupt disabled, PLL selected, Clock Monitor Disabled, VCO 0
                                      Divider=30(48MHZ->120MHz) */
#define MCG_C7_IN_PBE     0x02U    /* Unchanged from FBE mode */
#define OSC_CR_IN_PBE     0x80U    /* Unchanged from FBE mode */
#else
#define MCG_C1_IN_PBE     0x9EU    /* Unchanged from FBE mode */
#define MCG_C2_IN_PBE     0x94U    /* Unchanged from FBE mode */
#define MCG_C5_IN_PBE     0x41U    /* PLL Clock Enabled, PLL disabled in all Stop modes, PLL SrcDiv=2(8MHz->4MHz */
#define MCG_C6_IN_PBE     0x66U    /* Loss of Lock Interrupt disabled, PLL selected, Clock Monitor Enabled, VCO 0
                                      Divider=30(4MHZ->120MHz) */
#define MCG_C7_IN_PBE     0x00U    /* Unchanged from FBE mode */
#define OSC_CR_IN_PBE     0x80U    /* Unchanged from FBE mode */
#endif

/******************************************************************************
 * Register settings for PEE Mode
 ******************************************************************************/
#if ( IRC48MCLK == 1 )
#define MCG_C1_IN_PEE     0x3AU    /* Changes from PBE mode: MCGOUTCLK Src=PLL, FLLIRCSel=ExtRefClk,IntRefStopEn=False*/
#define MCG_C2_IN_PEE     0xA5U    /* Changes from PBE mode: IRCSel=Fast */
#define MCG_C5_IN_PEE     0x4BU    /* Unchanged from PBE mode */
#define MCG_C6_IN_PEE     0x46U    /* Unchanged from PBE mode */
#define MCG_C7_IN_PEE     0x02U    /* Unchanged from PBE mode */
#define OSC_CR_IN_PEE     0x80U    /* Unchanged from PBE mode */
#else
#define MCG_C1_IN_PEE     0x1AU    /* Changes from PBE mode: MCGOUTCLK Src=PLL, FLLIRCSel=ExtRefClk,IntRefStopEn=False*/
#define MCG_C2_IN_PEE     0xA5U    /* Changes from PBE mode: IRCSel=Fast */
#define MCG_C5_IN_PEE     0x41U    /* Unchanged from PBE mode */
#define MCG_C6_IN_PEE     0x66U    /* Unchanged from PBE mode */
#define MCG_C7_IN_PEE     0x00U    /* Unchanged from PBE mode */
#define OSC_CR_IN_PEE     0x80U    /* Unchanged from PBE mode */
#endif

/******************************************************************************
 * SIM Register settings for PEE Mode
 ******************************************************************************/
#define SIM_SOPT1_VALUE   0x00080000    /* LPTMR_32KOscClkSel=RTC32K, all other "read-only" */
#define SIM_SOPT2_VALUE   0x00050000    /* PLL/FLLClkSel=PLL, USBClk=PLL, all other remain at default/dont' care  */

#endif

/* ****************************************************************************************************************** */
/* Type Definitions */
/*lint -esym(751,enum_TestCase_t)   */
/*lint -esym(750,LED_INDICATOR_DELAY_MS)   */
/*lint -esym(749,enum_TestCase_t::TESTCASE_1,enum_TestCase_t::TESTCASE_2,enum_TestCase_t::TESTCASE_3,enum_TestCase_t::TESTCASE_4)   */
/*lint -esym(749,enum_TestCase_t::TESTCASE_5,enum_TestCase_t::TESTCASE_6,enum_TestCase_t::TESTCASE_7)   */
typedef void (application_t) (void);

typedef struct vector
{
    uint32_t        stack_addr;     // intvec[0] is initial Stack Pointer
    application_t   *func_p;        // intvec[1] is initial Program Counter
} vector_t;

typedef enum
{
   TESTCASE_1 = ((uint8_t)0), // initial test case just after initializing clock and hardware for bootloader application
   TESTCASE_2,                // just before we update DFWinfo partition, bootloader will redo entire process after power cycle
   TESTCASE_3,                // reserved, not utilized yet
   TESTCASE_4,                // reserved, not utilized yet
   TESTCASE_5,                // reserved, not utilized yet
   TESTCASE_6,                // reserved, not utilized yet
   TESTCASE_7                 // reserved, not utilized yet
}enum_TestCase_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */
#if ( MCU_SELECTED == NXP_K24 )
extern const vector_t   app_vector; // Application vector address symbol from the linker configuration file
#elif ( MCU_SELECTED == RA6E1 )
extern void hal_boot_app(void);     // Jump to the Application located after the block protection swap memory (0x4000)
#endif

/* ****************************************************************************************************************** */
/* File Scope Variables Definition */
static uint8_t  sectorData[ EXT_FLASH_ERASE_SIZE ];

#if ( MCU_SELECTED == RA6E1 )
/* CRC32 configuration structure */
crc32Cfg_t crcCfg;
#endif

/* ****************************************************************************************************************** */
/* Function Prototypes */
       void BL_DefaultISR( void );
static void BL_MAIN_RunApp ( void );
static void BL_MAIN_ClockSetup(void);
static void BL_MAIN_HWInit(void);
static void BL_MAIN_delayForStablePower(void);
static void BL_MAIN__msDelay(uint16_t mSeconds);
#if BOOTLOADER_UT_ENABLE
static void BootloaderUnitTest(enum_TestCase_t testCase);
#endif  //endif BOOTLOADER_UT_ENABLE

/* ****************************************************************************************************************** */
/* Function Definitions */

/*******************************************************************************

  Function Name:   BL_DefaultISR
  Purpose:         This is the ISR function for all unassigned interrupts
  Arguments:       void
  Returned Value:  void
  Notes:
    Unassigned interrupts are unexpected, so spin until WDOG expires

*******************************************************************************/

void BL_DefaultISR( void )
{
   for ( ;; )
   {
   }
}  /* End BL_DefaultISR() */


/*******************************************************************************

  Function Name:   BL_MAIN_RunApp
  Purpose:         This function is used to Execute the Application
  Arguments:       void
  Returned Value:  void
  Notes:           This function NEVER returns

*******************************************************************************/
static void BL_MAIN_RunApp ( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   __disable_interrupt();              // 1. Disable interrupts
   __set_SP( app_vector.stack_addr );  // 2. Configure stack pointer
   SCB_VTOR = ( uint32_t )&app_vector; // 3. Configure VTOR
   __enable_interrupt();               // 4. Enable interrupts
   app_vector.func_p();                // 5. Jump to application

#elif ( MCU_SELECTED == RA6E1 )
   /* look-up the application stack pointer and PC from its vector table at 0x4000 and jump to it */
   hal_boot_app();
#endif
}  /* End BL_MAIN_RunApp() */

/*******************************************************************************

  Function Name:   BL_MAIN_ClockSetup
  Purpose:         Configures the main processor clocks
  Arguments:       void
  Returned Value:  void
  Notes:

*******************************************************************************/
static void BL_MAIN_ClockSetup( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   // Perform Clock configurations (120/60/24/24MHz) {Saves code space in the App}
   /* Disable the USB Regulator */
   SIM_SOPT1CFG |= SIM_SOPT1CFG_URWE_MASK;         /* First allow USBREGEN bit to be written - SMG from 0x00 to 0x00 */
   SIM_SOPT1    &= ~( SIM_SOPT1_USBREGEN_MASK );   /* write this bit clears URWE - SMG from 0x8B010 to 0x8B010 */
   SIM_SCGC5    |= ( uint16_t )SIM_SCGC5_PORTA_MASK; /* Enable GPIO Port where EXTAL/XTAL pins exist */

#if ( IRC48MCLK == 1 )
   SIM_SCGC4 |= SIM_SCGC4_USBOTG_MASK;              /* Enable USB Clock Gate Control */
   USB0_CLK_RECOVER_IRC_EN = 0x02;                  /* Enable the IRC48M module */

   /* Make the PINs 18 and 19 as output PINs to disable the oscillations of the crystal */
   PORTA_PCR18 = 0x100;   /* PORTA_PCR18: MUX=1 */
   GPIOA_PDDR |= (1<<18); /* Make PORTA_PCR18 as Output PIN */
   GPIOA_PCOR |= (1<<18); /* Clear PORTA_PCR18 */
   PORTA_PCR19 = 0x100;   /* PORTA_PCR19: MUX=1 */
   GPIOA_PDDR |= (1<<19); /* Make PORTA_PCR19 as Output PIN */
   GPIOA_PCOR |= (1<<19); /* Clear PORTA_PCR19 */
#else
   PORTA_PCR18 &= ( uint32_t )~( uint32_t )0x0700;  /* PORTA_PCR18: MUX=0 */
   PORTA_PCR19 &= ( uint32_t )~( uint32_t )0x0700;  /* PORTA_PCR19: MUX=0 */
#endif

   /* Clock mode and configuration from Reset:
      FEI mode with Core/Sys and Bus clocks = ~21MHz, FlexBus and Flash clocks = ~10.5MHz.
      To get to PEE first must transition to FBE mode, then to PBE mode, then finally to PEE mode. */

   /* Change system pre-scalers to safe values:
      Core/Sys clock=~21MHz, BusClk=~10.5MHz, FlexBus and Flash clocks=~4.2MHz */
   SIM_CLKDIV1 = ( uint32_t )0x01440000;

   /* Transition from FEI to FBE */
   MCG_C2 = MCG_C2_IN_FBE | ( MCG_C2 & MCG_C2_FCFTRIM_MASK ); /* Set C2 (Leave fast IRC Fine Trim unchanged) */
   MCG_C1 = MCG_C1_IN_FBE;
   OSC_CR = OSC_CR_IN_FBE;
#if ( IRC48MCLK == 1 )
   MCG_C7 = MCG_C7_IN_FBE;   /* Select OSCCLK1(IRC48M) as the MCG FLL external reference clock */
#else
   /* Wait until the External oscillator is running/stable */
   while( ( MCG_S & MCG_S_OSCINIT0_MASK ) == 0x00U )
   {
   }
#endif

   /* Check that the source of the FLL reference clock is the external reference clock. */
   while( ( MCG_S & MCG_S_IREFST_MASK ) != 0x00U )
   {
   }

   MCG_C4 = ( MCG_C4_IN_FBE & ( uint8_t )( ~( MCG_C4_FCTRIM_MASK | MCG_C4_SCFTRIM_MASK ) ) ) |
            ( MCG_C4 & ( MCG_C4_FCTRIM_MASK | MCG_C4_SCFTRIM_MASK ) ); /* Set C4 (Leave trim values unchanged) */
   MCG_C5 = MCG_C5_IN_FBE;
   MCG_C6 = MCG_C6_IN_FBE;
   /* Wait until external reference clock is selected as MCG output */
   while( ( MCG_S & 0x0CU ) != 0x08U )
   {
   }

   /* Going from FBE to PBE */
   OSC_CR = OSC_CR_IN_PBE;
   MCG_C7 = MCG_C7_IN_PBE;
   MCG_C1 = MCG_C1_IN_PBE;
   MCG_C2 = MCG_C2_IN_PBE | ( MCG_C2 & MCG_C2_FCFTRIM_MASK ); /* Set C2 ((Leave fast IRC Fine Trim unchanged) */
   MCG_C5 = MCG_C5_IN_PBE;
   MCG_C6 = MCG_C6_IN_PBE;

   /* Wait until PLL is locked */
   while( ( MCG_S & MCG_S_LOCK0_MASK ) == 0x00U )
   {
   }

   /* Wait until external reference clock is selected as MCG output */
   while( ( MCG_S & 0x0CU ) != 0x08U )
   {
   }

   /* Going from PBE to PEE */
   OSC_CR = OSC_CR_IN_PEE;
   MCG_C7 = MCG_C7_IN_PEE;
   MCG_C1 = MCG_C1_IN_PEE;
   MCG_C2 = MCG_C2_IN_PEE | ( MCG_C2 & MCG_C2_FCFTRIM_MASK );
   MCG_C5 = MCG_C5_IN_PEE;
   MCG_C6 = MCG_C6_IN_PEE;

   /* Wait until PLL is locked */
   while( ( MCG_S & MCG_S_LOCK0_MASK ) == 0x00U )
   {
   }

   /* Wait until output of the PLL is selected */
   while( ( MCG_S & 0x0CU ) != 0x0CU )
   {
   }

   //SMG Doubt this is necessary since never use USB
   /* SIM_CLKDIV2: USBDIV=1,USBFRAC=0 */
   SIM_CLKDIV2 =  ( uint32_t )0x09UL; /* Update USB clock prescalers */
   SIM_CLKDIV1 =  ( uint32_t )0x01440000; /* Update system prescalers */
   SIM_SOPT1   =  ( uint32_t )( ( SIM_SOPT1 & ( uint32_t )~( uint32_t )SIM_SOPT1_OSC32KSEL_MASK ) |
                                ( uint32_t )SIM_SOPT1_VALUE ); /* Update 32 kHz oscillator clock source (ERCLK32K) */
   SIM_SOPT2   =  ( uint32_t )( ( SIM_SOPT2 & ( uint32_t )~( uint32_t )SIM_SOPT2_PLLFLLSEL_MASK ) |
                                ( uint32_t )SIM_SOPT2_VALUE ); /* Update PLL/FLL clock select */

   SYST_CSR = (SysTick_CSR_ENABLE_MASK | SysTick_CSR_CLKSOURCE_MASK); /* needed systick enabled to count cycles for needed delays */
   SYST_RVR = SYST_RVR_VALUE; /* set the reload register */
#elif ( MCU_SELECTED == RA6E1 )
   // clock is setup in SystemInit, do nothing
#endif
}  /* End BL_MAIN_ClockSetup() */

/*******************************************************************************

  Function Name:   BL_MAIN_HWInit
  Purpose:         Configure the bare minimum HW pins configurations
  Arguments:       void
  Returned Value:  void
  Notes:

*******************************************************************************/
static void BL_MAIN_HWInit( void )
{
   /* device specific hardware setup */
#if ( MCU_SELECTED == NXP_K24 )
   // Configure active PORTx for GPIO
   /* Enable clock gating to all ports */
   SIM_SCGC5 |=   SIM_SCGC5_PORTA_MASK \
                | SIM_SCGC5_PORTB_MASK \
                | SIM_SCGC5_PORTC_MASK \
                | SIM_SCGC5_PORTD_MASK \
                | SIM_SCGC5_PORTE_MASK;

   SIM_SCGC6 |= SIM_SCGC6_CRC_MASK; /* Enable clock gating to CRC */

   /* Avoid EZP_CS/NMI functionality for the PTA4 pin  */
   PORTA_PCR4 = (uint32_t)((PORTA_PCR4 & (uint32_t)~(uint32_t)(PORT_PCR_MUX_MASK)) | (uint32_t)(PORT_PCR_MUX(0x01)));
   //  {/SPI0_CS_FLASH, PORTA bit 14 - SPI driver to set up
   //  {SPI0_SCK,       PORTA bit 15 - SPI driver to set up
   //  {SPI0_MOSI,      PORTA bit 16 - SPI driver to set up
   //  {SPI0_MISO,      PORTA bit 17 - SPI driver to set up
   //  {EXTAL0,         PORTA bit 18 - setup during clock configuration
   //  {XTAL0,          PORTA bit 19 - setup during clock configuration
   //  {3V6LDO_EN,      PORTB bit 1
   //  {3V6BOOST_EN,    PORTC bit 0
   //
   //  {/WP_FLASH,      PORTB bit 18 - SPI driver to set up
   //  {/PF_METER,      PORTD bit 0
   BRN_OUT_TRIS();
   //  {UART2_RX_DBG,   PORTD bit 2 - UART driver to set up
   //  {UART2_TX_DBG,   PORTD bit 3 - UART driver to set up
   GRN_LED_PIN_TRIS();
   RED_LED_PIN_TRIS();
   BLU_LED_PIN_TRIS();

#elif ( MCU_SELECTED == RA6E1 )
   // hardware clocks and GPIO are setup in R_BSP_WarmStart, do nothing
#endif
   /* common hardware setup */
   PWR_3P6LDO_EN_TRIS_ON();
   PWR_3V6BOOST_EN_TRIS_OFF();
}  /* End BL_MAIN_HWInit() */

#if 0
/* Debug only replacement for memcmp that gives address of 1st difference. */
flAddr doDiff( uint8_t *ramBuf, uint8_t *romData, uint32_t len )
{
   flAddr result = 0;

   while ( ( len != 0 ) && ( *ramBuf == *romData ) )
   {
      ramBuf++;
      romData++;
      len--;
   }
   if ( len != 0 )
   {
      result = ( flAddr )romData;
   }
   return result;
}

/* Debug only byte by byte comparison of NV to code.  */
flAddr compareNVtoCode( flAddr codeAddr, PartitionData_t const * pPartition, uint32_t cnt )
{
   flAddr   diffAddr = 0;
   uint32_t partitionOffset = 0;
   uint32_t operationCnt;

   while( 0 != cnt )
   {
      operationCnt = sizeof( sectorData );   /* The current operation will operate on the size of sectorData */
      if ( operationCnt > cnt )              /* Are the remaining bytes less than the sectorData size? */
      {
         /* Yes, only operate on cnt bytes */
         operationCnt = cnt;
      }
      if ( eSUCCESS != PAR_partitionFptr.parRead( &sectorData[0], codeAddr + partitionOffset, operationCnt,
                                                      pPartition ) ) /* Rd x bytes */
      {
         break;
      }
      diffAddr = doDiff( sectorData, ( uint8_t *)(codeAddr + partitionOffset), operationCnt );
      if ( diffAddr != 0 )
      {
         break;
      }
      cnt             -= operationCnt; /* Adjust count */
      partitionOffset += operationCnt; /* Adjust offset into both regions */
   }
   return diffAddr;
}
/* Debug only copy of CRC32_calc used in the DFW_app  */
uint32_t CRC32_calc(void const *pData, uint32_t cnt, crc32Cfg_t *pCrcCfg)
{
   uint8_t *pDataVal = (uint8_t *)pData;
   if (cnt)
   {
      do
      {
         uint8_t index = 8;
         uint8_t dataByte = *pDataVal++;
         do
         {
            if ((pCrcCfg->crcVal & 1) ^ (dataByte & 1))
            {
               pCrcCfg->crcVal >>= 1;
               pCrcCfg->crcVal ^= pCrcCfg->poly;
            }
            else
               pCrcCfg->crcVal >>= 1;
            dataByte >>= 1;
         }while(--index);
      }while(--cnt);
   }

   uint32_t calCrc;
   if (pCrcCfg->invertResult)
   {
      calCrc = ~pCrcCfg->crcVal;
   }
   else
   {
      calCrc = pCrcCfg->crcVal;
   }
   return(calCrc);
}

void CRC32_init(uint32_t startVal, uint32_t poly, eCRC32_Result_t invertResult, crc32Cfg_t *pCrcCfg)
{
   pCrcCfg->crcVal = startVal;
   pCrcCfg->poly = poly;
   pCrcCfg->invertResult = invertResult;
}

/********************************************************************
* 32bit data CRC Calculation
********************************************************************/
static returnStatus_t calcCRC(flAddr StartAddr, uint32_t cnt, bool start, uint32_t * const pResultCRC,
                                PartitionData_t const *pPartition)
{
   returnStatus_t retVal = eSUCCESS;   /* return value */
   crc32Cfg_t     crcCfg;              /* CRC configuration - Don't change elements in this structure!!! */
   uint32_t       operationCnt;        /* Number of bytes to operate on (read and perform CRC32) per pass of loop */
   uint8_t        buffer[32];          /* Buffer to store the read data from partition x */
   flAddr         partitionOffset = StartAddr; /* Start CRC32 at the beginning of the partition */

   if ( start )
   {
      *pResultCRC = CRC32_DFW_START_VALUE;
   }
   CRC32_init(*pResultCRC, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg); /* Init CRC */

   while(0 != cnt)
   {
      operationCnt = sizeof(buffer);   /* The current operation will operate on the size of buffer */
      if (operationCnt > cnt)          /* Are the remaining bytes less than the buffer size? */
      {  /* Yes, only operate on cnt bytes */
         operationCnt = cnt;
      }
      if (eSUCCESS != PAR_partitionFptr.parRead(&buffer[0], partitionOffset, operationCnt, pPartition)) /* Rd x bytes */
      {
         retVal = eFAILURE;
         break;
      }
      *pResultCRC      = CRC32_calc(&buffer[0], operationCnt, &crcCfg); /* Update the CRC */
      cnt             -= operationCnt;                                  /* Adjust count */
      partitionOffset += operationCnt;                                  /* Adjust the offset into the partition */
   }
   return(retVal);
}
#endif

#if ( MCU_SELECTED == NXP_K24 )
/*******************************************************************************

  Function Name:  bitReverse
  Purpose:        Bit reverses a 32 bit value. Taken from "Bit Twiddling Hacks"
  Arguments:      uint32_t x  value to bit reverse

  Returned Value: bit reversed vale of x

*******************************************************************************/
static uint32_t bitReverse( uint32_t x )
{
   x = ( ( ( x & 0xaaaaaaaa ) >> 1 ) | ( ( x & 0x55555555 ) << 1 ) );   /* swap odd and even bits  */
   x = ( ( ( x & 0xcccccccc ) >> 2 ) | ( ( x & 0x33333333 ) << 2 ) );   /* swap consectutive pairs */
   x = ( ( ( x & 0xf0f0f0f0 ) >> 4 ) | ( ( x & 0x0f0f0f0f ) << 4 ) );   /* swap nibbles            */
   x = ( ( ( x & 0xff00ff00 ) >> 8 ) | ( ( x & 0x00ff00ff ) << 8 ) );   /* swap bytes              */
   return( ( x >> 16 ) | ( x << 16 ) );                                 /* swap 2-byte long pairs  */

}
#endif

#if ( MCU_SELECTED == NXP_K24 )
/* Cleaned up version of mqx version of crc_kn.c */
/*******************************************************************************

  Function Name:  do_CRC
  Purpose:        Run CRC calculation with preset parameters in CRC engine. Allows "continuation" operation, assuming no
                  pre-emption, which is valid in the bootloader environment.
  Arguments:      uint8_t *input - pointer to data to run through CRC engine
                  uint32_t length - number of bytes to run through CRC engine

  Returned Value: computed CRC

   Notes: assumes that the entire length of input data is directly accessible (e.g., not partition reads req'd.

*******************************************************************************/
static uint32_t do_CRC( const uint8_t *input, uint32_t length )
{
   CRC_MemMapPtr  crcDevice = CRC_BASE_PTR;
   uint32_t       sizeDwords;
   uint32_t       in32;
   uint32_t       u32cnt;
   uint32_t       idx;

   /* Load as many 32bit values as possible. */
   sizeDwords = length >> 2;
   idx = 0;
   for( u32cnt = 0; u32cnt < sizeDwords; u32cnt++ )
   {
      in32 = ( (  (uint32_t)input[idx] << 24 ) |
                  (uint32_t)( input[idx + 1] << 16 ) |
                  (uint32_t)( input[idx + 2] << 8 ) |
                  (uint32_t)( input[idx + 3] ) );
      idx += 4;
      crcDevice->DATA = in32;
   }
   /* If not a multiple of 4 bytes, finish by writing individual bytes  */
   while( idx < length )
   {
      *( uint8_t * )&crcDevice->DATA = input[idx];
      idx++;
   }
   return crcDevice->DATA;
}
#elif ( MCU_SELECTED == RA6E1 )
/* CRC32 calculation (matching DFW) provided by crc32.c */
#endif

/*******************************************************************************

  Function Name:  bl_crc32
  Purpose:        Performs CRC32 that is compatible with the DFW application
  Arguments:      uint32_t seed           CRC seed value
                  uint32_t poly           CRC polynomial
                  uint8_t *input          pointer to input data
                  uint32_t length         number of bytes over which to compute CRC
  Returned Value: resulting CRC32


*******************************************************************************/
static uint32_t bl_crc32( uint32_t seed, uint32_t poly, const uint8_t *input, uint32_t length )
{
#if ( MCU_SELECTED == NXP_K24 )
   /* configure CRC calculation */
   CRC_MemMapPtr  crcDevice = CRC_BASE_PTR;
   uint32_t       totr  = 0x2;   /* Data read from engine have bits AND bits reversed  */
   uint32_t       tot   = 1;     /* Data writteng to engine have bytes transposed      */
   uint32_t       fxor  = 1;     /* Result is inverted ( XOR with 0xffffffff )         */

   /* Set CRC engine parameters.  */
   crcDevice->CTRL = CRC_CTRL_TCRC_MASK | ( tot << 30 ) | ( totr << 28 ) | ( fxor << 26 );
   crcDevice->GPOLY = bitReverse( poly );    /* Set the polynomial value. Note: CRC engine requires this to be bit reversed */

   crcDevice->CTRL |= CRC_CTRL_WAS_MASK;     /* Input data, Set WaS=1   */
   crcDevice->DATA = seed;                   /* Set the seed value.  */
   crcDevice->CTRL &= ~CRC_CTRL_WAS_MASK;    /* Input data, Set WaS=0   */

   return do_CRC( input, length );
#elif ( MCU_SELECTED == RA6E1 )
   /* configure CRC calculation */
   crc32Cfg_t crcCfg;
   CRC32_init(seed, poly, eCRC32_RESULT_INVERT, &crcCfg);

   /* calculate CRC */
   return CRC32_calc(input, length, &crcCfg);
#endif
}
/*******************************************************************************

  Function Name:   main
  Purpose:         This is the Bootloaders main function
  Arguments:       void
  Returned Value:  As required by compiler
  Notes:           This function NEVER returns

*******************************************************************************/
#if ( MCU_SELECTED == NXP_K24 )
int main( void )
#elif ( MCU_SELECTED == RA6E1 )
int BL_MAIN_Main( void )
#endif
{
   PartitionData_t const * pDFWinfo;      /* Partition handle for DFW information   */
   PartitionData_t const * pNVinfo;       /* Partition handle for whole external NV */
   PartitionData_t const * pCodeInfo;     /* Partition handle for whole code space  */
   flAddr                  CRCAddr;       /* CRC starting address.   */
   dSize                   CRCLength;     /* CRC range.              */
   uint32_t                SrcAddr;       /* Local copy of DFWinfo SrcAddr */
   uint32_t                DstAddr;       /* Local copy of DFWinfo DstAddr */
   uint32_t                Length;        /* Local copy of DFWinfo Length */
   uint32_t                expectedCRC;   /* CRC calculated by DFW app. */
   uint32_t                calculatedCRC; /* CRC calculated by bootloader app. */
   uint32_t                bytesCopied;   /* Number of bytes copied from ext. NV to int. flash. */
   dSize                   DFWInfoIdx = 0;/* Loop counter (number of ranges copied) */
   DfwBlInfo_t             DFWinfo[ MAX_COPY_RANGES ];
   bool                    success = ( bool )true;
   bool                    copyAttempted = ( bool )false;

   WDOG_Disable();

   __enable_interrupt();   //TODO: Figure out how to enable interrupts besides this call...

//   printf( "Hello from bootloader!\n" );

#if ( MCU_SELECTED == NXP_K24 )
   // If ACKISO is active, then must be coming out of reset from Last Gasp (VLLS1) and can ignore DFW requests for now
   if ( PMC_REGSC & PMC_REGSC_ACKISO_MASK )
   {
      //   Doing Last Gasp so run application
      BL_MAIN_RunApp();
   }
#elif ( MCU_SELECTED == RA6E1 )
   // add equivalent code to check for last gasp active to bypass update check
#endif

   //OK, we are going to run the Bootloader!
   BL_MAIN_ClockSetup();
   BL_MAIN_HWInit();
   ( void )WDOG_Init();

   /* Test Case: Initial */
   BOOTLOADER_TEST(TESTCASE_1);
   BL_MAIN_delayForStablePower(); /* check to make sure power is stable before continue*/

   /* Read the DFW Info partition to determine if there is an application or bootloader update pending.  */
   ( void ) PAR_init();

   /* Verify DFW Bootloader partition is accessible.  */
   if ( eSUCCESS == PAR_partitionFptr.parOpen( &pDFWinfo, ePART_DFW_BL_INFO, 0 ) )
   {
      /* Verify both external NV and internal NV are accessible.  */
      if ( eSUCCESS == PAR_partitionFptr.parOpen( &pNVinfo, ePART_DFW_PGM_IMAGE, 0 )  &&
         ( eSUCCESS == PAR_partitionFptr.parOpen( &pCodeInfo, ePART_APP_CODE, 0 ) ) )
      {
         /* check for possible bootloader info */
         while ( ( DFWInfoIdx < ARRAY_IDX_CNT( DFWinfo ) ) )  /* Loop while in valid DFWinfo[] range and */
         {
#if ( MCU_SELECTED == RA6E1)
            /* is DFW INFO blank in the DFW BL INFO partition? */
            if ( eSUCCESS == PAR_partitionFptr.parBlankCheck(
                                                PART_DFW_BL_INFO_DATA_OFFSET + ( DFWInfoIdx * sizeof( DfwBlInfo_t ) ),
                                                sizeof( DfwBlInfo_t ), pDFWinfo ))
            {
               /* blank indicates no update, skip */
            }
            else
#endif
            {
               /* Read an update segment from the DFW BL INFO partition.   */
               if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&DFWinfo[ DFWInfoIdx ],
                                                   PART_DFW_BL_INFO_DATA_OFFSET + ( DFWInfoIdx * sizeof( DfwBlInfo_t ) ),
                                                   sizeof( DfwBlInfo_t ), pDFWinfo ))
               {
                  /* update info length indicates an image is available? */
                  if ( DFWinfo[ DFWInfoIdx ].Length != 0xffffffff )  /* If there's data to copy. */
                  {
                     /* collect update info and begin update */
                     CRCAddr     =  DFWinfo[ DFWInfoIdx ].DstAddr;   /* Save CRC start addr, length and expected value. */
                     CRCLength   =  DFWinfo[ DFWInfoIdx ].Length;
                     expectedCRC =  DFWinfo[ DFWInfoIdx ].CRC;

                     SrcAddr     =  DFWinfo[ DFWInfoIdx ].SrcAddr;   /* Save src addr, dst addr and length for loop. */
                     DstAddr     =  DFWinfo[ DFWInfoIdx ].DstAddr;   /* Save CRC start addr, length and expected value. */
                     Length      =  DFWinfo[ DFWInfoIdx ].Length;    /* Save CRC start addr, length and expected value. */
                     if ( DFWinfo[ DFWInfoIdx ].FailCount == 0xffffffff )   /* If errCount is "erased", set to 0.  */
                     {
                        DFWinfo[ DFWInfoIdx ].FailCount = 0;
                     }

                     while ( Length != 0 )    /* Loop until all bytes in the range have been copied.  */
                     {
                        /* Calculate data to be copied per pass.  */
                        bytesCopied = min( EXT_FLASH_ERASE_SIZE, Length );
                        (void) PAR_partitionFptr.parRead( ( uint8_t * )&sectorData[0], SrcAddr, bytesCopied, pNVinfo  );
                        (void) PAR_partitionFptr.parWrite( DstAddr, ( uint8_t * )&sectorData[0], bytesCopied, pCodeInfo );
                        Length  -= bytesCopied;
                        SrcAddr += bytesCopied;
                        DstAddr += bytesCopied;
                        copyAttempted = ( bool )true;
                        WDOG_Kick();
                     }

#if 0
                     calculatedCRC = ( uint32_t )compareNVtoCode( pCodeInfo->PhyStartingAddress + CRCAddr, pNVinfo, CRCLength );
                     calcCRC(  CRCAddr, CRCLength, true, &calculatedCRC, pCodeInfo );
#endif
                     /* Validate CRC.
                        Src, Dst values from DFWinfo are offsets into their respective partitions. Translate for use in CRC
                        validation. Physical code address is partition starting address + value from DFWinfo struct.
                     */
                     calculatedCRC = bl_crc32(  CRC32_DFW_START_VALUE,                                /* Seed  */
                                                CRC32_DFW_POLY,                                       /* Polynomial */
                                                (uint8_t *)(pCodeInfo->PhyStartingAddress + CRCAddr), /* Starting addr. */
                                                CRCLength );                                          /* Length   */

                     if ( calculatedCRC != expectedCRC )
                     {
                        DFWinfo[ DFWInfoIdx ].FailCount++;
                        success = ( bool )false;
                     }
                  }
               }
            }

            DFWInfoIdx++;  /* Bump to next update segment */
         }
      }
   }

   /* Update the DFWinfo in ROM. */
   if ( copyAttempted )
   {
      /* Test Case: After flash write completed, but DFWinfo had not been updated yet */
      BOOTLOADER_TEST(TESTCASE_2);
      BL_MAIN_delayForStablePower(); /* check to make sure power is stable before write */

      /*lint -e{645} if copyAttemped is true, DFWinfo has been initialized.   */
      if ( eSUCCESS != PAR_partitionFptr.parWrite( PART_DFW_BL_INFO_DATA_OFFSET, ( uint8_t * )DFWinfo,
                                                   sizeof( DFWinfo ), pDFWinfo ) )
      {
         success = ( bool ) false;
      }
   }
   if ( success )
   {
      BL_MAIN_RunApp();    /* Everything copied successfully, run the main application.   */
   }
   RESET(); /* Try again!  */
}

#if BOOTLOADER_UT_ENABLE
/***********************************************************************************************************************
 *
 * Function Name: BootloaderUnitTest
 *
 * Purpose: Signal via LED's to the tester to perform any reset recovery testing at critical sections of the
 *          boot loader code execution.
 *
 * Arguments: enum_TestCase_t test sequence number - Determines the LED configuration for a specific test case.
 *
 * Returns: None
 *
 * Note:
 *    1. This function is called using the "BOOTLOADER_TEST" macro.
 *    2. Each call will have a different test case enum supplied as the parameter.  Based upon the test case
 *       supplied in the function call, a different LED on configuration will be indicated to the tester.
 *    3. Three LED's will be viewed as a sequence of bits: GreenLed = bit 0, RedLed = bit 1, BlueLed = bit 2.
 *       With this method, an LED sequence will be lit according the the value of test case parameter passed to
 *       the function.  For example, to indicate code execution is currently in test case #5, bit 0(GreenLed) and
 *       bit 2(BlueLed) will be turned on.
 *    4. Since number of test cases will be limited to number of LED's on/off combinations, the maximum amount of
 *       test cases will be 7.  Any invalid test case arguments  will have no affect on the LED behavior.
 *    5. When a test case is encountered, the approriate illuminated LED configuration will be displayed the user.
 *       A delay will be started to allow time for the tester to perform any actions needed for testing.  If the
 *       tester does not peform any actions and the delay period expires, the code will resume until it encounters
 *       the next test case.
 *
 * Side Effects: Multisecond Delay is introduced into code execution to allow time for tester to observe led status
 *               and perform any necessary actions.
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
static void BootloaderUnitTest( enum_TestCase_t testCase )
{

  /* change led state based upon testCase value */
  switch ( testCase )
  {
      case TESTCASE_1:
      {
         GPIOE_PSOR = LED0_PIN; /* turn on green led */
         break;
      }
      case TESTCASE_2:
      {
         GPIOE_PSOR = LED1_PIN; /* turn on red led */
         break;
      }
      case TESTCASE_3:
      {
         GPIOE_PSOR = LED0_PIN; /* turn on green led */
         GPIOE_PSOR = LED1_PIN; /* turn on red led */
         break;
      }
      case TESTCASE_4:
      {
         GPIOE_PSOR = LED2_PIN; /* turn on blue led */
         break;
      }
      case TESTCASE_5:
      {
         GPIOE_PSOR = LED0_PIN; /* turn on green led */
         GPIOE_PSOR = LED2_PIN; /* turn on blue led */
         break;
      }
      case TESTCASE_6:
      {
         GPIOE_PSOR = LED1_PIN; /* turn on red led */
         GPIOE_PSOR = LED2_PIN; /* turn on blue led */
         break;
      }
      case TESTCASE_7:
      {
         GPIOE_PSOR = LED0_PIN; /* turn on green led */
         GPIOE_PSOR = LED1_PIN; /* turn on red led */
         GPIOE_PSOR = LED2_PIN; /* turn on blue led */
         break;
      }
      default:
      {
         break;
      }
   }

   /* create delay to allow tester to visual see LED's and perform needed test operation */
   BL_MAIN__msDelay( (uint16_t)LED_INDICATOR_DELAY_MS );

   /* verify all led's are off before next test case */
   GPIOE_PCOR = LED0_PIN;
   GPIOE_PCOR = LED1_PIN;
   GPIOE_PCOR = LED2_PIN;
}
#endif  //endif BOOTLOADER_UT_ENABLE


/***********************************************************************************************************************

   Function Name: BL_MAIN_delayForStablePower

   Purpose: Debounce the brown-out signal until indicates power is good

   Arguments:  None

   Returns: None

   Side Effects: This will block until the brown-out signal until indicates power is good. Clears WDT.

   Reentrant Code: No

 **********************************************************************************************************************/
static void BL_MAIN_delayForStablePower(void)
{
   uint8_t debounceCount = 0; /* counter to track successive brownout signal checks */

   while( debounceCount < DEBOUNCE_CHECK_LIMIT ) /* 10 steps through while loop will give us 100ms check of power fail signal */
   {
      WDOG_Kick();
      BL_MAIN__msDelay( (uint16_t)DEBOUNCE_CHECK_MS_DELAY); /* 10ms delay between checks of power fail signal */

      if ( BRN_OUT() ) /* are we in power fail condition? */
      {
         debounceCount = 0;
      }
      else
      {
         ++debounceCount;
      }
   }
}

/***********************************************************************************************************************

   Function Name: BL_MAIN__msDelay

   Purpose: Debounce the brown-out signal until indicates power is good

   Arguments:  None

   Returns: None

   Side Effects: This will block until the brown-out signal until indicates power is good. Clears WDT.

   Reentrant Code: No

 **********************************************************************************************************************/
static void BL_MAIN__msDelay(uint16_t mSeconds)
{
#if ( MCU_SELECTED == RA6E1 )
   R_BSP_SoftwareDelay(mSeconds, BSP_DELAY_UNITS_MILLISECONDS);
#elif ( MCU_SELECTED == NXP_K24 )
   uint32_t initialSysTick = 0; /* variable to store initial systick value */
   uint32_t currentSystick = 0; /* variable to track change in systick valuet to compare to initial */
   bool msTickOccurred; /* flag used to stop do/while loop */
   uint16_t counter = 0; /* counter to track mseconds */

   while( counter < mSeconds)
   {
      msTickOccurred = false;
#if ( MCU_SELECTED == NXP_K24 )
      initialSysTick = SYST_CVR; /* update the initial systick value */
#elif ( MCU_SELECTED == RA6E1 )
      initialSysTick = SysTick->VAL; /* update the initial systick value */
#endif

      do /* spin here until 120000 system ticks have occured appr. 1ms */
      {
#if ( MCU_SELECTED == NXP_K24 )
         currentSystick = SYST_CVR; /* get current systick value */
#elif ( MCU_SELECTED == RA6E1 )
         currentSystick = SysTick->VAL; /* get current systick value */
#endif
         /* systick value counts down from SYS_RVR to 0 */
         if( initialSysTick > currentSystick ) /* if initial systick value is larger than current, systick reload has not occured */
         {
            if( initialSysTick - currentSystick > SYSTICK_CYCLES_1MS ) /* has 1ms worth of clock cycles occured? */
            {
               msTickOccurred = true;
            }
         }
         else  /* current systick value is larger than intitial value, systick reload has occured */
         {
#if ( MCU_SELECTED == NXP_K24 )
            if( ( initialSysTick + ( SYST_RVR - currentSystick ) ) > SYSTICK_CYCLES_1MS ) /* has 1ms worth of clock cycles occured? */
#elif ( MCU_SELECTED == RA6E1 )
            if( ( initialSysTick + ( SysTick->LOAD - currentSystick ) ) > SYSTICK_CYCLES_1MS ) /* has 1ms worth of clock cycles occured? */
#endif
            {
               msTickOccurred = true;
            }
         }

      } while(!msTickOccurred); /* has 1ms of systick time passed?  If so, stop loop */

      counter++; /* update the ms counter */
      /* Note: This only relevant for the delays greater than or equal to 1 second */
      if( counter % 1000 == 0 ) /* kick the watchdog after 1000 ms */
      {
         WDOG_Kick();
      }

   }
#endif  /* NXP_K24 */
}
