/***********************************************************************************************************************

   Filename:   spi_mstr.c

   Global Designator: SPI_

   Contents: SPI driver for the pic24 controller.  This driver uses the SPI hardware in the pic24.

 ***********************************************************************************************************************
   Copyright (c) 2013-2015 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$ kdavlin Created January 15, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 01/15/2013 - Initial Release

 ***********************************************************************************************************************
   Note:  The DMA version is designed to use an RTOS.
   Note:  See notes on lower power mode in the function header, SPI_pwrMode().
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project_BL.h"
#include <stdbool.h>
#include "spi_mstr_BL.h"
#include "sys_clock_BL.h"
#ifndef __BOOTLOADER
#include <bsp.h>
#include "DBG_SerialDebug.h"
#else
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#include <MK66F18.h>
#else
#include <MK24F12.h>
#endif
#endif   /* BOOTLOADER  */

#ifndef __BOOTLOADER
#include "portable_aclara.h"
#endif   /* __BOOTLOADER   */

#ifdef TM_SPI_DRIVER_UNIT_TEST
#include "print.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define SPI_KHz_TO_Hz(kHz) ((uint32_t)kHz*(uint32_t)1000) /* Converts from kHz to Hz */
#ifndef __BOOTLOADER
#define SPI_ISR_EN         (true)                         /* Flag to enable the SPI ISR */
#endif   /* __BOOTLOADER   */
#define SPI_CLK_SCALER     ((uint32_t)100000)             /* Used to calc SPI Clk without using floating point math */
#define SPI_FRAME_SIZE     ((uint8_t)7)                   /* 8-bit transfer rate, 7 = 8 bits */

#ifndef __BOOTLOADER
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )   || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84020_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84030_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84114_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y99852_1_REV_A ) )
#define SPI_RADIO_PORT 1  // Radio SPI port on Samwise
#elif ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )|| \
        ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )  )
#define SPI_RADIO_PORT 0  // RX radios SPI port on Frodo
#else
#error "Unsupported TARGET HARDWARE in spi_mstr.c"
#endif
#endif   /* __BOOTLOADER   */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#define USE_MQX_SPI_DRIVER 0

#if USE_MQX_SPI_DRIVER == 0
#ifndef __BOOTLOADER
static returnStatus_t SPI_WriteReadPort( uint8_t port, uint8_t const *pTxData, uint8_t *pRxData, uint16_t cnt );
static void SpiDmaIsr( uint8_t port );
void Spi0DmaIsr( void );
void Spi1DmaIsr( void );
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
void Spi2DmaIsr( void );
#endif
#endif
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
#define SPI_COUNT 3
#else
#define SPI_COUNT 2
#endif

/*lint -esym(551, spiPortName)   Not used in non-mqx condition, except to know number of valid SPI ports */
#ifndef __BOOTLOADER
static const char * const spiPortName[SPI_COUNT] =
{
   "spi0:",
   "spi1:"
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   "spi2:"
#endif
};
#endif

// pSpiX_ - Spi base Address for each channel.
/* Each SPI has a group of SFRs.  This is a list of pointers to each group of SFRs for each SPI port. */
#if USE_MQX_SPI_DRIVER == 0
static const spiPort_t * const pSpiX_[SPI_COUNT] =
{
   ( spiPort_t * )( void * )SPI0_BASE_PTR, /* Address of SPI 0 SFRs */
   ( spiPort_t * )( void * )SPI1_BASE_PTR /* Address of SPI 1 SFRs */
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   ( spiPort_t * )( void * )SPI2_BASE_PTR /* Address of SPI 2 SFRs */
#endif
}; /*lint !e740  unusual cast is okay. */

//primaryPrescaleValues_ - Prescale for SPI clock calculations.
static const uint8_t  primaryPrescaleValues_[] = { 2, 3, 5, 7 }; /* Primary Prescaler's valid values. */

//Baud Rate Values - BR value for SPI Clock Calculations.
/* Baud Rate valid values. */
static const uint16_t baudRateValues_[] =
{ 2, 4, 6, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };

//clockGate_ - Configures Clock each SPI port channel
static const sysClkGateCtrlWithMask_t clockGate_[SPI_COUNT] =  /* Configures the Clock for each SPI port channel */
{
   { ( uint32_t volatile * ) & SIM_SCGC6, SIM_SCGC6_SPI0_MASK }, // System Clock Gating Control Register 6, bit 12
   { ( uint32_t volatile * ) & SIM_SCGC6, SIM_SCGC6_SPI1_MASK }  // System Clock Gating Control Register 6, bit 13
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   { ( uint32_t volatile * ) & SIM_SCGC3, SIM_SCGC3_SPI2_MASK }  // System Clock Gating Control Register 3, bit 12
#endif
};

// spiChPinCtrlCfg_t - Pin Configuration for all SPI ports
/*lint -e{835} -e{446}  at this time warning 446 has not been documented  9/25/2013 */
#endif
static const spiChPinCtrlCfg_t spiPinCfg_[SPI_COUNT] =
{
   {  /* SPI Port 0 */ // Control serial flash on EPs. Radios on 9975T and 9985T
      { SPI_PORT_0_CLK_PIN,  SPI_PORT_0_CLK_PIN_MUX },  //SPI0_SCK
      { SPI_PORT_0_MOSI_PIN, SPI_PORT_0_MOSI_PIN_MUX }, //SPI0_SOUT
      { SPI_PORT_0_MISO_PIN, SPI_PORT_0_MISO_PIN_MUX }, //SPI0_SIN
      { SPI_PORT_0_CS_PIN,   SPI_PORT_0_CS_PIN_MUX }    //SPI0_PCS0
   },
   {  /* SPI Port 1 */ // Control radio on EPs. Serial Flash on 9975T, Flash and Backplane on 9985T
      { SPI_PORT_1_CLK_PIN,  SPI_PORT_1_CLK_PIN_MUX },  //SPI1_SCK
      { SPI_PORT_1_MOSI_PIN, SPI_PORT_1_MOSI_PIN_MUX }, //SPI1_SOUT
      { SPI_PORT_1_MISO_PIN, SPI_PORT_1_MISO_PIN_MUX }, //SPI1_SIN
      { SPI_PORT_1_CS_PIN,   SPI_PORT_1_CS_PIN_MUX }    //SPI1_PCS1
   }
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   {  /* SPI port 2 */ // Backplane on 9975T
      { SPI_PORT_2_CLK_PIN,  SPI_PORT_2_CLK_PIN_MUX },  //SPI2_SCK
      { SPI_PORT_2_MOSI_PIN, SPI_PORT_2_MOSI_PIN_MUX }, //SPI2_SOUT
      { SPI_PORT_2_MISO_PIN, SPI_PORT_2_MISO_PIN_MUX }, //SPI2_SIN
      { SPI_PORT_2_CS_PIN,   SPI_PORT_2_CS_PIN_MUX }    //SPI2_PCS2
   }
#endif
};

// spiDmaCfg_ - Defines all of the parameters for SPI DMA cfg
#if USE_MQX_SPI_DRIVER == 0
/*lint -e{835} */
#if SPI_USES_DMA == 1
static const spiDmaIsrCfg_t spiDmaCfg_[SPI_COUNT] =         /* Defines all of the parameters needed for SPI DMA configuration */
{
   {  /* SPI Port 0 */
      SPI0_TX_DMA_CH_ISR,   SPI0_RX_DMA_CH_ISR,
      ( INT_ISR_FPTR )Spi0DmaIsr,                  /* DMA CH Interrupt, DMA ISR Function */
      SPI_ISR_PRIORITY,    SPI_ISR_SUBPRIORITY,    /* DMA ISR Priority, DMA ISR Sub Priority */
      SPI0_TX_DMA_CH,      SPI0_RX_DMA_CH,         /* DMA CH for TX, DMA CH for RX */
      SPI0_TX_DMA_SRC,     SPI0_RX_DMA_SRC         /* DMA TX Src, DMA RX Src */
   },
   {  /* SPI Port 1 */
      SPI1_TX_DMA_CH_ISR,   SPI1_RX_DMA_CH_ISR,
      ( INT_ISR_FPTR )Spi1DmaIsr,                  /* DMA CH Interrupt, DMA ISR Function */
      SPI_ISR_PRIORITY,    SPI_ISR_SUBPRIORITY,    /* DMA ISR Priority, DMA ISR Sub Priority */
      SPI1_TX_DMA_CH,      SPI1_RX_DMA_CH,         /* DMA CH for TX, DMA CH for RX */
      SPI1_TX_DMA_SRC,     SPI1_RX_DMA_SRC         /* DMA TX Src, DMA RX Src */
   }
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   {  /* SPI Port 2 */
      SPI2_TX_DMA_CH_ISR,   SPI2_RX_DMA_CH_ISR,
      ( INT_ISR_FPTR )Spi2DmaIsr,                  /* DMA CH Interrupt, DMA ISR Function */
      SPI_ISR_PRIORITY, SPI_ISR_SUBPRIORITY,       /* DMA ISR Priority, DMA ISR Sub Priority */
      SPI2_TX_DMA_CH,   SPI2_RX_DMA_CH,            /* DMA CH for TX, DMA CH for RX */
      SPI2_TX_DMA_SRC,  SPI2_RX_DMA_SRC            /* DMA TX Src, DMA RX Src */
   }
#endif
};

static const uint8_t LinkChan[SPI_COUNT] =
{
   SPI0_TX_LINK_CHAN,
   SPI1_TX_LINK_CHAN
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
   ,
   SPI2_TX_LINK_CHAN
#endif
};
#endif
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#ifndef __BOOTLOADER
static bool rtosEn_ = ( bool )true;                            /* true = use RTOS semaphores */
#endif   /* BOOTLOADER  */

#if USE_MQX_SPI_DRIVER == 0
/*lint -esym(551,txByte_)  txByte_ is used with DMA */
static uint8_t       txByte_[ARRAY_IDX_CNT( pSpiX_ )];         /* Byte to tx when rx data from the external device. */
#ifndef __BOOTLOADER
static ePowerMode  pwrMode_ = ePWR_NORMAL;                     /* Sets the power mode to normal at init. */

#if SPI_USES_DMA == 1
static OS_SEM_Obj spiSem_[ARRAY_IDX_CNT( pSpiX_ )];            /* Semaphore given when SPI has completed a transfer */
static bool SpiSemCreated[ARRAY_IDX_CNT( pSpiX_ )] = { false };
static volatile bool spiIsrFired_[ARRAY_IDX_CNT( pSpiX_ )];    /* true = ISR fired, false = ISR has not fired. */
#endif
static OS_MUTEX_Obj spiMutex_[ARRAY_IDX_CNT( pSpiX_ )];        /* Mutex Lock for each channel */
static const spiCfg_t* spiCurrentCfg[ARRAY_IDX_CNT(pSpiX_)] = {0};   /* Pointers to the constant cfg arrays for each SPI in use */
#if USE_MQX_SPI_DRIVER != 0
static MQX_FILE_PTR spifd[ARRAY_IDX_CNT( spiPortName )];       /* MQX file info for the spi ports  */
#endif
#endif   /* BOOTLOADER  */
#endif

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: SPI_MutexLock

   Purpose: Prevent interference between multiple device [drivers] operating on the smae SPI port

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc)

   Returns:

   Side Effects:

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void SPI_MutexLock(uint8_t port)
{
   // Don't use mutex when accessing radios
   // SPI for radios is not shared so skipping mutex access will be faster.
   if ( port != SPI_RADIO_PORT )
   {
      OS_MUTEX_Lock( &spiMutex_[port] );  // Function will not return if it fails
   }
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_MutexUnlock

   Purpose:

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc)

   Returns:

   Side Effects:

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void SPI_MutexUnlock(uint8_t port)
{
   // Don't use mutex for when accessing radios
   // SPI for radios is not shared so skipping mutex access will be faster.
   if ( port != SPI_RADIO_PORT )
   {
      OS_MUTEX_Unlock( &spiMutex_[port] );   // Function will not return if it fails
   }
}
#endif

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: SPI_initPort

   Purpose: Initializes the SPI port

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc )

   Returns: eSPI_Status_t - eSPI_SUCCESS or eSPI_FAILURE

   Side Effects:

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
returnStatus_t SPI_initPort( uint8_t port )
{
   returnStatus_t retVal = eFAILURE;

   if ( port < ARRAY_IDX_CNT( spiPortName ) )     /* If 0 is passed in, the decrement would make it roll under. */
   {
#if USE_MQX_SPI_DRIVER == 0
#if SPI_USES_DMA == 1
      if ( SpiSemCreated[port] == false )  /* Is the sem not valid (not created)? */
      {
         //TODO RA6: NRJ: determine if semaphores need to be counting
         if ( true == OS_SEM_Create( &spiSem_[port], 0 ) ) // Create the semaphore
         {
            SpiSemCreated[port] = true;
            /* Setup the ISR for the DMA transfer complete */
            if ( NULL != _int_install_isr( ( uint32_t )spiDmaCfg_[port].isrTxIndx, spiDmaCfg_[port].isr, ( void * )NULL ) )
            {
               ( void )_bsp_int_init( ( uint32_t )spiDmaCfg_[port].isrTxIndx, spiDmaCfg_[port].pri - 1,
                                      spiDmaCfg_[port].subPri, ( bool )SPI_ISR_EN );
               if ( NULL != _int_install_isr( ( uint32_t )spiDmaCfg_[port].isrRxIndx, spiDmaCfg_[port].isr, ( void * )NULL ) )
               {
                  ( void )_bsp_int_init( ( uint32_t )spiDmaCfg_[port].isrRxIndx, spiDmaCfg_[port].pri,
                                         spiDmaCfg_[port].subPri, ( bool )SPI_ISR_EN );
                  if ( true == OS_MUTEX_Create( &spiMutex_[port] ) )
                  {
                     retVal = eSUCCESS;
                  }
               }
            }
         }
      }
      else  /* semaphore handle has already be created. */
      {
         retVal = eSUCCESS;
      }
#else
      retVal = eSUCCESS;
#endif
#endif
      retVal = eSUCCESS;   /*lint !e838 previous value not used; OK  */
   }
   return ( retVal );
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_SetFastSlewRate

   Purpose: Configures the slew rate

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc )
               fastRate: True: set slew rate to fast.

   Returns: none

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void SPI_SetFastSlewRate( uint8_t port, bool fastRate ) /*lint -e{835} */
{
   if ( fastRate == true )
   {
      // Enable fast slew rate
      *spiPinCfg_[port].clk.pPinCtrlReg  &= ~PORT_PCR_SRE_MASK; /* Set the Clock GPIO Pin */
      *spiPinCfg_[port].mosi.pPinCtrlReg &= ~PORT_PCR_SRE_MASK; /* Set the MOSI GPIO Pin */
   }
   else
   {
      // Enable slow slew rate
      *spiPinCfg_[port].clk.pPinCtrlReg  |= PORT_PCR_SRE_MASK; /* Set the Clock GPIO Pin */
      *spiPinCfg_[port].mosi.pPinCtrlReg |= PORT_PCR_SRE_MASK; /* Set the MOSI GPIO Pin */
   }
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_OpenPort

   Purpose: Configures the following:
      Serial clock bit rate
      Clock Phase
      Clock Polarity
      Transmit byte (dummy byte when receiving)
      The SPI port is configured for 8-bit mode.

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc )
               pSpiCfg: Structure that configures the SPI Channel.
               master: True for master, false for slave

   Returns: eSPI_Status_t -
            eSPI_SUCCESS - Configuration accepted
            eSPI_BUSY - Can't configure the SPI port while a transaction is taking place.
            eSPI_INVALID_BIT_RATE - Can't configure the bit rate requested.  The bit rate is requested is slower
            than can be achieved.
            eSPI_INVALID_BITS - Invalid number of bits to send.

   Side Effects: Internal SPI registers are affected.

   Reentrant Code: No

   Notes:  Only CTAR0 is used for the SPI port.
           Port mutex must be held while function is executed

 **********************************************************************************************************************/
returnStatus_t SPI_OpenPort( uint8_t port, spiCfg_t const * const pCfg, bool master ) /*lint -e{835} */
{
   returnStatus_t eRetVal = eSPI_INVALID_PORT;  /* Assume an invalid port was passed in. */
#if USE_MQX_SPI_DRIVER == 0
   spiPort_t      *pSpi;                        /* Points to the appropriate SPI port SFRs. */

   /*lint --e{456} if OS_MUTEX_Lock call fails, the mutex is NOT taken   */
   /*lint --e{454} if OS_MUTEX_Lock call succeeds, the mutex IS taken, AND unlocked   */
   if ( port < ARRAY_IDX_CNT( pSpiX_ ) )        /* Valid port number? */
   {
#ifndef __BOOTLOADER
      spiCurrentCfg[port] = pCfg;               /* Store the pointer to the current config */
#endif
      eRetVal = eSUCCESS;                       /* The port is valid, return success. */
      txByte_[port] = pCfg->txByte;             /* Byte to send when receiving data */

      pSpi = ( spiPort_t * )pSpiX_[port];       /* Get a pointer to the proper SPI port index. */

      /* Configure the SPI Port Pins */
      *clockGate_[port].pSysClkGateCtrl |= clockGate_[port].mask;       /* Set the clock gate */
      /* Clear the configuration and turn off the SPI port before setting individual fields. */
      pSpi->mcr.reg = SPI_MCR_HALT_MASK | SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK | SPI_MCR_ROOE_MASK;
      if (master) {
         pSpi->mcr.reg |= SPI_MCR_MSTR_MASK;
      }
      *spiPinCfg_[port].clk.pPinCtrlReg = spiPinCfg_[port].clk.value;   /* Set the Clock GPIO Pin */
      *spiPinCfg_[port].miso.pPinCtrlReg = spiPinCfg_[port].miso.value; /* Set the MISO GPIO Pin */
      *spiPinCfg_[port].mosi.pPinCtrlReg = spiPinCfg_[port].mosi.value; /* Set the MOSI GPIO Pin */
      if ( 0 != spiPinCfg_[port].cs.value )                             /* Use the Chip Select GPIO Pin? */
      {
         *spiPinCfg_[port].cs.pPinCtrlReg = spiPinCfg_[port].cs.value;  /* Set the CS GPIO Pin */
      }

      pSpi->rser.reg = 0;                       /* Disable the interrupts */
      pSpi->sr.reg = 0;                         /* Clear all status flags */
      pSpi->sr.bits.tfff = 1;                   /* set buffer empty flag */
      pSpi->sr.bits.tcf = 1;                    /* Set transfer complete flag */
      pSpi->ctar0.reg = 0;                      /* Clear the config in CTAR. */
      pSpi->ctar0.bits.fmsz = SPI_FRAME_SIZE;   /* Set the SPI port for 8-bit transfers (fmsz = x + 1) */
      pSpi->mcr.bits.pcsis = 0x3f;              /* All active low chip selects. 1 = Active Low, 0 = Active High */

      if (master) {
         /* Calculate the bit rate, Formula:  SpiClock = (SysClock/PBR)*((1+DBR)/BR)
            The 100000 is put in to avoid floating point math.  The loops below will calculate the closest bit rate
            to the target value passed in without going over the target value passed in.  This may not be the
            quickest way to calculate this, but it should only be done once at power up. */
         uint32_t   dbrMult;             /* Double the Clock Rate (0 = Don't multiply, 1 = Multiply by 2) */
         uint32_t   closestBitRate;      /* Calculated Closest Bit Rate   */
         uint32_t   targetBitRateHz = SPI_KHz_TO_Hz( pCfg->clkBaudRate_Khz ); /* Convert the target clock rate to Hz */
         for ( dbrMult = SPI_CLK_SCALER, closestBitRate = 0; dbrMult < ( SPI_CLK_SCALER * 3 ); dbrMult += SPI_CLK_SCALER )
         {
            uint8_t    pbrIdx; /* Index into the PBR values */
            for ( pbrIdx = 0; pbrIdx < ARRAY_IDX_CNT( primaryPrescaleValues_ ); pbrIdx++ )
            {
               uint8_t   brIdx; /* Baud Rate Scaler */
               for ( brIdx = 0; brIdx < ARRAY_IDX_CNT( baudRateValues_ ); brIdx++ )
               {
                  /* Calculate what the bitrate would be given the values: DBR, PBR and BR */
                  uint32_t bitRate = ( ( getBusClock() / primaryPrescaleValues_[pbrIdx] ) / SPI_CLK_SCALER ) *
                                     ( dbrMult / ( baudRateValues_[brIdx] ) );
                  if ( SPI_MAX_SPEED_MHz >= bitRate ) /* is the bitrate less than or equal to the max allowed? */
                  {
                     if ( bitRate > closestBitRate ) /* Is the new bitrate faster than the previous closest bitrate? */
                     {
                        if ( ( targetBitRateHz - bitRate ) < ( targetBitRateHz - closestBitRate ) ) /* Closer to target? */
                        {
                           closestBitRate = bitRate;                             /* Capture the new bit rate */
                           pSpi->ctar0.bits.br = brIdx;                          /* Set the BR value */
                           pSpi->ctar0.bits.dbr = ( dbrMult / SPI_CLK_SCALER ) - 1; /* Remove the scaler and sub 1 */
                           pSpi->ctar0.bits.pbr = pbrIdx;                        /* Set the PBR value */
                           break;
                        }
                     }
                     else  /* The bitrate is slower than the closest to the target.  Stop trying larger dividers. */
                     {
                        break;
                     }
                  }
               }
            }
         }
      }
      // Configure the SPI mode

      pSpi->ctar0.bits.cpha = ( pCfg->mode & 1 );   /* Mode 1 or 3 sets SMP (clock phase) bit. */
      pSpi->ctar0.bits.cpol = ( pCfg->mode & 2 );   /* Mode 2 or 3 sets the CKP (clock polarity) bit. */
#if SPI_USES_DMA == 1

      SPI_DMA_CLK_CFG(); /* Enables the DMA's Clocks */

      // Description of each register used below:
      /* DMA TX Configuration:  See Section 22.3.16 in the K60 Reference Manual, K60P144M150SF3RM
            SADDR:  The source addr is set in the write function
            SOFF:  Source Address Signed Offset - Sign-extended offset applied to the current source address to
                  form the next-state value as each source read is completed.  Set to 0.
            ATTR: TCD Transfer Attributes
                  SMOD: Source Address Modulo, 0 Source address modulo feature is disabled
                  SSIZE: Source data transfer size - 000 = 8-bit
                  DMOD: Destination Address Modulo
                  DSIZE: Destination Data Transfer Size
            MLNO:  TCD Minor Byte Count (Minor Loop Disabled) - TCD word 2's register definition depends on the
                  status of minor loop mapping.
            MLOFFNO:  TCD Signed Minor Loop Offset (Minor Loop Enabled and Offset Disabled)
                  SMLOE: Source Minor Loop Offset Enable
                  DMLOE: Destination Minor Loop Offset enable
                  NBYTES: Minor Byte Transfer Count
            MLOFFYES:  TCD Signed Minor Loop Offset (Minor Loop and Offset Enabled)
                  SMLOE: Source Minor Loop Offset Enable
                  DMLOE: Destination Minor Loop Offset Enable
                  MLOFF: If SMLOE or DMLOE is set, this field represents a sign-extended offset applied to the
                  source or destination address to form the next-state value after the minor loop completes.
                  NBYTES: Minor Byte Transfer Count, # of bytes to be transferred in each service request of the
                  ch.
            SLAST: TCD Last Source Address Adjustment - Adjustment value added to the source address at the
                  completion of the major iteration count. This value can be applied to restore the source address
                  to the initial value, or adjust the address to reference the next data structure.
            DADDR:  TCD Destination Address - The destination addr is set to the SPI PUSHR register.
            DOFF:  TCD Signed Destination Address Offset
                  DOFF: Sign-extended offset applied to the current destination address to form the next-state
                  value as each destination write is completed.
            CITER_ELINKYES:  TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled)
                  ELINK: Enable channel-to-channel linking on minor-loop complete As the channel completes the
                  minor loop, this flag enables linking to another channel, defined by the LINKCH field. The link
                  target channel initiates a channel service request via an internal mechanism that sets the
                         TCDn_CSR[START] bit of the specified channel.
                  LINKCH: Link Channel Number -  If channel-to-channel linking is enabled (ELINK = 1), then after
                  the minor loop is exhausted, the eDMA engine initiates a channel service request to the channel
                  defined by these six bits by setting that channel's TCDn_CSR[START] bit.
                  CITER: Current Major Iteration Count - This 9-bit (ELINK = 1) or 15-bit (ELINK = 0) count
                  represents the current major loop count for the channel.  It is decremented each time the minor
                  loop is completed and updated in the transfer control descriptor memory. After the major
                  iteration count is exhausted, the channel performs a number of operations (e.g., final source
                  and destination address calculations), optionally generating an interrupt to signal channel
                  completion before reloading the CITER field from the beginning iteration count (BITER) field.
            CITER_ELINKNO:  TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled)
                  ELINK: Enable channel-to-channel linking on minor-loop complete - As the channel completes the
                  minor loop, this flag enables linking to another channel, defined by the LINKCH field. The link
                  target channel initiates a channel service request via an internal mechanism that sets the
                         TCDn_CSR[START] bit of the specified channel.
                  CITER: Current Major Iteration Count - This 9-bit (ELINK = 1) or 15-bit (ELINK = 0) count
                  represents the current major loop count for the channel. It is decremented each time the minor
                  loop is completed and updated in the transfer control descriptor memory. After the major
                  iteration count is exhausted, the channel performs a number of operations (e.g., final source
                  and destination address calculations), optionally generating an interrupt to signal channel
                  completion before reloading the CITER field from the beginning iteration count (BITER) field.
            DLAST_SGA:  TCD Last Destination Address Adjustment/Scatter Gather Address - Destination last address
                        adjustment or the memory address for the next transfer control descriptor to be loaded
                        into this channel (scatter/gather).
            CSR:  DMA_TCDn_CSR field descriptions:
                  BWC: Bandwidth Control
                  MAJORLINKCH: Link Channel Number
                  DONE: Channel Done
                  ACTIVE: Channel Active
                  MAJORLINK: Enable channel-to-channel linking on major loop complete
                  ESG: Enable Scatter/Gather Processing
                  DREQ: Disable Request
                  INTHALF: Enable an interrupt when major counter is half complete.
                  INTMAJOR: Enable an interrupt when major iteration count completes
                  START: Channel Start
            BITER_ELINKYES:  TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled)
                  ELINK: Enables channel-to-channel linking on minor loop complete
                  LINKCH: Link Channel Number
                  BITER: Starting Major Iteration Count
            TCDO_BITER: TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled)
                  ELINK: Enables channel-to-channel linking on minor loop complete
                  BITER: Starting Major Iteration Count
      */
      /* TFFF DMA requests are enabled,  TFFF_DIRS Transmit FIFO Fill DMA, Transmit FIFO Underflow Request Enable,
         Receive FIFO Drain Request Enable, Receive FIFO Drain DMA, TCF interrupt requests are enabled */
      pSpi->rser.reg =  SPI_RSER_TFFF_RE_MASK | SPI_RSER_TFFF_DIRS_MASK | SPI_RSER_TFUF_RE_MASK |
                        SPI_RSER_RFDF_RE_MASK | SPI_RSER_RFDF_DIRS_MASK | SPI_RSER_TCF_RE_MASK;
      pSpi->mcr.bits.disRxf = 1;    /* Disable the RX FIFO.  */
      pSpi->mcr.bits.disTxf = 1;    /* Disable the TX FIFO */

      pSpi->mcr.bits.rooe = 1;      /* Receive incoming bytes. */

      /* *** Set up the DMA for TX *** */
      /* Configure the SPI port to generate DMA requests.  The settings below don't change once configured. */
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].txDmaCh )  = 0; /* disable before changing any values */
      DMA_ATTR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;       /* Sets DSIZE and SIZE */
      DMA_NBYTES_MLNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1; /* Transfer 1 byte */
      DMA_CITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1; /* One loop per request   */
      DMA_BITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1; /* One loop per request   */
      DMA_SLAST_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;      /* Don't adjust after last byte sent */
      DMA_DADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = ( uint32_t )&pSpi->pushr.reg;
      DMA_DOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;       /* Don't increment the dest offset */
      DMA_DLAST_SGA_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;  /* Don't adjust after last byte sent */
      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;
      /* Setup the DMA MUX to route the proper DMA source to the specified DMA channel */
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = DMAMUX_CHCFG_SOURCE( LinkChan[port] );
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].txDmaCh ) |= DMAMUX_CHCFG_ENBL_MASK;

      /* *** Set up the DMA for RX *** */
      /* Config DMA channel to receive data from SPI port. Settings below don't change once configured. */
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].rxDmaCh )  = 0; /* disable before changing any values */
      DMA_ATTR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;       /* Sets DSIZE and SIZE */
      DMA_NBYTES_MLNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 1; /* Xfr 1 byte (8 bits) with each loop */
      /* Set link bit, use TxDmaCh, one loop per request   */
      DMA_CITER_ELINKYES_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) =
         DMA_CITER_ELINKYES_ELINK_MASK |
         ( uint16_t )( spiDmaCfg_[port].txDmaCh << DMA_CITER_ELINKYES_LINKCH_SHIFT ) |
         1;
      DMA_BITER_ELINKYES_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) =
         DMA_BITER_ELINKYES_ELINK_MASK |
         ( uint16_t )( spiDmaCfg_[port].txDmaCh << DMA_BITER_ELINKYES_LINKCH_SHIFT ) |
         1;
      DMA_SLAST_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;
      DMA_SADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = ( uint32_t )&pSpi->popr;
      DMA_SOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;    /* Set of TCD Signed Source Address Offset */
      DMA_DLAST_SGA_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;
      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;
      /* Setup the DMA MUX to route the proper DMA source to the specified DMA channel */
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = DMAMUX_CHCFG_SOURCE( spiDmaCfg_[port].rxDmaSrc );
      DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) |= DMAMUX_CHCFG_ENBL_MASK;
#endif

      pSpi->mcr.bits.halt = 0;      /* Enable the SPI port */
   }
#else
   if ( spifd[ port ] == NULL )
   {
      spifd[ port ] = fopen( spiPortName[ port ], SPI_FLAG_HALF_DUPLEX );
      if ( spifd[ port ] != NULL )
      {
         eRetVal = eSUCCESS;
      }
   }
   else
   {
      eRetVal = eSUCCESS;
   }
#endif
   return ( eRetVal );
}

/***********************************************************************************************************************

   Function Name: SPI_ClosePort

   Purpose: Disables the SPI Driver, all I/O pins return to their default state.

   Arguments:  port:  SPI port number (1, 2, etc.)
               NOTE:  parameter is 1 based to match the datasheet.

   Returns: eSPI_Status_t - eSPI_SUCCESS or eSPI_FAILURE

   Side Effects: Internal SPI registers are effected.

   Reentrant Code: Yes - mutex protected

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
returnStatus_t SPI_ClosePort( uint8_t port )
{
   returnStatus_t eRetVal = eSPI_INVALID_PORT; /* Initialize the response, assume a failure */
#if USE_MQX_SPI_DRIVER == 0

   if ( port < ARRAY_IDX_CNT( pSpiX_ ) )
   {
      /* To do */

      eRetVal = eSUCCESS;
   }
   return ( eRetVal );
#else
   if ( spifd[ port ] != NULL )
   {
      if ( fclose( spifd[ port ] ) != 0 )
      {
         eRetVal = eSPI_INVALID_PORT;
      }
      else
      {
         spifd[ port ] = NULL;
         eRetVal = eSUCCESS;
      }
   }
   return ( eRetVal );
#endif
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_ChkSharedPortCfg

   Purpose: To be called immediately after SPI_MutexLock() to make sure SPI port is using the desired config

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc)
               pCfg: Pointer to the desired operating configuration

   Returns:

   Side Effects: If the desired config doesn't match the current port config, the port is re-opened with the desired config

   Reentrant Code: No

   Notes: This was going to be bundled into the SPI_MutexLock() function, but it was decided for readbility and visibility
          reasons that it would be better as a separate function.

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void SPI_ChkSharedPortCfg(uint8_t port, const spiCfg_t* const pCfg)
{
   // Don't use mutex when accessing radios
   // SPI for radios is not shared so skipping mutex access will be faster.
   if ( (port != SPI_RADIO_PORT) && (pCfg != spiCurrentCfg[port]) ) //If not using the desired config, reopen the port with the correct config
   {
      (void)SPI_ClosePort(port);
      (void)SPI_OpenPort(port, pCfg, SPI_MASTER);
   }
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_misoCfg

   Purpose: Configures the MISO pin as a GPIO or SPI.  This is used for certain Flash devices that can use the MISO pin
            as a busy indicator.  For Example, SST devices.  This function allows the external flash driver to configure
            this pin as SPI or GPIO on the fly to detect the busy signal.

   Arguments:  port: Selects the SPI port number (0, 1, 2, etc )
               spiMisoCfg_e cfg

   Returns: eSUCCESS or eSPI_INVALID_PORT

   Side Effects: The spi port will change its configuration.

   Reentrant Code: Yes

   Notes:  N/A

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
returnStatus_t SPI_misoCfg( uint8_t port, spiMisoCfg_e cfg )
{
   returnStatus_t eRetVal = eSPI_INVALID_PORT;  /* Assume an invalid port was passed in. */

   if ( port < ARRAY_IDX_CNT( spiPortName ) )        /* Valid port number? */
   {
      eRetVal = eSUCCESS;                       /* The port is valid, return success. */
      if ( SPI_MISO_SPI_e == cfg )
      {
         *spiPinCfg_[port].miso.pPinCtrlReg = spiPinCfg_[port].miso.value; /* Set the MISO GPIO Pin for SPI */
      }
      else
      {
         *spiPinCfg_[port].miso.pPinCtrlReg = PORT_PCR_MUX( 1 ) | SPI_MISO_EN_PULLUP; /* Set the MISO GPIO Pin as a Digital Input */
      }
   }
   return ( eRetVal );
}
#endif

/***********************************************************************************************************************

   Function Name: SPI_pwrMode

   Purpose: Configures the SPI port to either put the processor to sleep while waiting for DMA to finish or not.

   Arguments:  spiPwrMode_t powerMode - Command to sleep or not

   Returns: None

   Side Effects: Any time the SPI is accessed in low power mode, the processor will sleep.

   Reentrant Code: No

   Notes: Put the processor to sleep, the DMA will wake up when the DMA ISR occurs.  This sleep mode needs to be set
   so that the peripheral devices are running.  Also, ALL OTHER INTERRUPTS MUST be disabled!!!

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
#if USE_MQX_SPI_DRIVER == 0
void SPI_pwrMode( ePowerMode powerMode )
{
   pwrMode_ = powerMode;
}
#endif
#endif


#if SPI_USES_DMA == 0
/* The following functions do NOT use DMA.  The board package header file contains this definition. */

/***********************************************************************************************************************

   Function Name: SPI_WritePort

   Purpose: Sends cnt number of bytes out the SPI port.  The data received is not used and discarded.  The receive
   buffer is not read.  The received bytes are counted to ensure the transmission is complete before returning.  This
   allows the driver accessing the SPI port to raise its chip select after all data is sent and not before.

   Arguments:  port: Selects the SPI port number (1, 2, etc.)
               uint8_t *pTxData:  Points to the data to send
               uint16_t u16Len:  Number of bytes to transmit

   Returns: eSPI_SUCCESS or eSPI_FAILURE

   Side Effects: Internal SPI registers are affected.

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t SPI_WritePort( uint8_t port, uint8_t const *pTxData, uint16_t cnt )
{
   uint8_t dummy;
   spiPort_t *pSpi = ( spiPort_t * )pSpiX_[port]; /* Points to the appropriate SPI port SFRs. */

   while( cnt-- )
   {
      // wait write buffer not full flag
      while ( !( pSpi->sr.bits.tfff ) )
      {};

      pSpi->pushr.reg = SPI_PUSHR_TXDATA( *pTxData++ );

      while ( !pSpi->sr.bits.tcf )      // while shift-out complete
      {};
      dummy = ( uint8_t )pSpi->popr;    // Read the dummy received data
      dummy = dummy;                    // Suppress Compiler warning
      pSpi->sr.reg = SPI_SR_TCF_MASK;   // clear flag
   }
   return ( eSUCCESS );
}

/***********************************************************************************************************************

   Function Name: SPI_ReadPort

   Purpose: Receives a cnt number of bytes from the SPI port.  The RX buffer is used to transmit the data.  The
   configuration will determine if the RX is preset to a defined value before the exchange begins.

   Arguments:  port: Selects the SPI port number (1, 2, etc.)
               uint8_t *pRxData:  Points to the data to send
               uint16_t u16Len:  Number of bytes to receive

   Returns: eSPI_Status_t - eSPI_SUCCESS or eSPI_FAILURE

   Side Effects: Internal SPI registers are effected.

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t SPI_ReadPort( uint8_t port, uint8_t *pRxData, uint16_t rxCnt )
{
   spiPort_t *pSpi = ( spiPort_t * )pSpiX_[port]; /* Points to the appropriate SPI port SFRs. */

   while( rxCnt-- )
   {
      // wait write buffer not full flag
      while ( !( pSpi->sr.bits.tfff ) )
      {};

      // Assert CS0, Use config 0
      pSpi->pushr.reg = SPI_PUSHR_PCS( 1 << ( 0 ) ) | SPI_PUSHR_CTAS( 0 ) | txByte_[port];

      while ( !pSpi->sr.bits.tcf )      // while shift-out complete
      {};
      *pRxData++ = ( uint8_t )pSpi->popr;
      pSpi->sr.reg = SPI_SR_TCF_MASK;   // clear flag
   }
   return ( eSUCCESS );
}

#else
/* The following functions use DMA. */

/***********************************************************************************************************************

   Function Name: SPI_WritePort

   Purpose: Sends cnt number of bytes out the SPI port using the DMA.  The data received is not used and discarded.

   Arguments:  port: Selects the SPI port number (1, 2, etc.)
               uint8_t *pTxData:  Points to the data to send
               uint16_t cnt:  Number of bytes to transmit

   Returns: eSPI_SUCCESS or eSPI_FAILURE

   Side Effects:

   Reentrant Code: yes

 **********************************************************************************************************************/
returnStatus_t SPI_WritePort( uint8_t port, uint8_t const *pTxData, uint16_t cnt )
{
   /* If using DMA on SPI 1 or 2, max read is 511 bytes. Loop if necessary   */
#if USE_MQX_SPI_DRIVER == 0
   returnStatus_t retVal = eSUCCESS;
   uint16_t maxCITER;
   uint16_t localCnt = cnt;

   maxCITER = DMA_CITER_ELINKYES_CITER_MASK;
   while ( cnt )
   {
      localCnt = min( cnt, maxCITER ); /* Don't try to read any more than maxCITER per loop! */
      if ( eSUCCESS != ( SPI_WriteReadPort( port, pTxData, NULL, localCnt ) ) )
      {
         retVal = eFAILURE;
         break;
      }
      cnt -= localCnt;
      pTxData += localCnt;
   }
#else
   returnStatus_t retVal = eFAILURE;
   if ( cnt == fwrite( ( void * )pTxData, 1, cnt, spifd[ port ] ) )
   {
      retVal = eSUCCESS;
   }
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function Name: SPI_ReadPort

   Purpose: Receives a cnt number of bytes from the SPI port use the DMA.  The RX buffer is used to transmit the data.

   Arguments:  port: Selects the SPI port number (1, 2, etc.)
               uint8_t *pRxData:  Points address to store the incoming data
               uint16_t cnt:  Number of bytes to receive

   Returns: eSPI_Status_t - eSPI_SUCCESS or eSPI_FAILURE

   Side Effects: Internal SPI registers are effected and DMA is used.

   Reentrant Code: yes

 **********************************************************************************************************************/
returnStatus_t SPI_ReadPort( uint8_t port, uint8_t *pRxData, uint16_t cnt )
{
   /* If using DMA on SPI 1 or 2, max read is 511 bytes. Loop if necessary   */
#if USE_MQX_SPI_DRIVER == 0
   returnStatus_t retVal = eSUCCESS;
   uint16_t maxCITER;
   uint16_t localCnt = cnt;

   maxCITER = DMA_CITER_ELINKYES_CITER_MASK;
   while ( cnt )
   {
      localCnt = min( cnt, maxCITER ); /* Don't try to read any more than maxCITER per loop! */
      if ( eSUCCESS != ( SPI_WriteReadPort( port, NULL, pRxData, localCnt ) ) )
      {
         retVal = eFAILURE;
         break;
      }
      cnt -= localCnt;
      pRxData += localCnt; /*lint !e662  pRxData will not exceed the bounds defined by 'cnt'   */
   }
#else
   returnStatus_t retVal = eFAILURE;
   if ( cnt == fread( ( void * )pRxData, 1, cnt, spifd[ port ] ) )
   {
      retVal = eSUCCESS;
   }
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function Name: SPI_WriteReadPort

   Purpose: Sends and/or Receives cnt bytes of data.  The interrupt is always driven from the RX data to ensure that all
            bytes are received before giving the semaphore.  The TX DMA will start the RX DMA transfer.

   Arguments:  port: Selects the SPI port number (1, 2, etc.)
               uint8_t *pRxData:  Points to the data to send
               uint8_t *pRxData:  Points to the data to receive
               uint16_t cnt:  Number of bytes to receive

   Returns: eSPI_Status_t - eSPI_SUCCESS or eSPI_FAILURE

   Side Effects: Internal SPI registers are effected and DMA is used.

   Reentrant Code: yes

 **********************************************************************************************************************/
#if USE_MQX_SPI_DRIVER == 0
static returnStatus_t SPI_WriteReadPort( uint8_t port, uint8_t const *pTxData, uint8_t *pRxData, uint16_t cnt )
{
   spiPort_t *pSpi = ( spiPort_t * )pSpiX_[port]; /* Points to the appropriate SPI port SFRs. */

   returnStatus_t retVal = eSUCCESS;   /* Assume success.  Failure can occur if the Sem times out. */
   /*lint --e{456} if OS_MUTEX_Lock call fails, the mutex is NOT taken   */
   /*lint --e{454} if OS_MUTEX_Lock call succeeds, the mutex IS taken, AND unlocked   */
   if ( 0 != cnt ) /* If 0 bytes are being sent, return eSUCCESS, else process the bytes */
   {
      volatile uint8_t rxData[2]; /*lint -esym(550,rxData)  Temp loc to receive data when received data is ignored. */

      /* ********* Configure the DMA for receiving data. ********** */
#if 0
      do /* This loop is to catch a condition where the DMA RX captures a byte before anything is transmitted. */
      {
         DMA_CERQ = spiDmaCfg_[port].rxDmaCh;                                          /* Clears Enable Request */
         DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;
         DMA_CITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = cnt;        /* Transfer cnt bytes */
         DMA_BITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = cnt;        /* Transfer cnt bytes */
         if ( NULL != pTxData )  /* Receiving Data to temp location (TX only mode)?   If so, don't inc buffer. */
         {
            DMA_DADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = ( uint32_t )&rxData[0]; /* Set dest address in DMA. */
            DMA_DOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;                /* Do NOT increment dest */
         }
         else
         {
            DMA_DADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = ( uint32_t )pRxData; /* Set dest address in the DMA. */
            DMA_DOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 1;                /* Increment dest */
         }
         DMA_SERQ = spiDmaCfg_[port].rxDmaCh;                                          /* Enable ch - Starts transfer */

      }while((DMA_CSR_ACTIVE_MASK & DMA_CSR_REG(DMA_BASE_PTR,spiDmaCfg_[port].rxDmaCh)) != 0 ); /* loop while active */
#endif

      DMA_CERQ = spiDmaCfg_[port].rxDmaCh;   /* Disable ch - Stops transfer */
      DMA_CERQ = spiDmaCfg_[port].txDmaCh;   /* Disable ch - Stops transfer */
      DMA_CINT = spiDmaCfg_[port].rxDmaCh;   /* Clear Rx channel pending interrupt  */
      DMA_CINT = spiDmaCfg_[port].txDmaCh;   /* Clear Tx channel pending interrupt  */

      spiIsrFired_[port] = ( bool )false;
      pSpi->sr.reg = 0xffffffff; /* Clear all SPI interrupts   */
      pSpi->rser.reg = SPI_RSER_RFDF_RE_MASK | SPI_RSER_RFDF_DIRS_MASK;

      /* ********* Configure the DMA for transmitting data. ********** */

      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0; /* Disable interrupts from the Rx channel */
      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0; /* Disable interrupts from the Tx channel */
#if 0
      DMA_CITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1;  /* 1 loop, 1 byte per loop */
      DMA_BITER_ELINKNO_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1;  /* 1 loop, 1 byte per loop */
#endif

      // Check if we are slave and transmitting
      if ( ( NULL != pTxData ) && ((pSpi->mcr.reg & SPI_MCR_MSTR_MASK) == 0) ) {
         // It looks like when SPI is slave, we need to send one more byte than expected
         // It may have to do with the fact that the first byte in the FIFO is the first one to go out but it is not the first byte we want to send.
         // The first byte we want to send will be loaded in the FIFO on the next turn.
         cnt++;
      }

      /* Set link bit, use TxDmaCh, one loop per request   */
      DMA_CITER_ELINKYES_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) =
         DMA_CITER_ELINKYES_ELINK_MASK |
         ( uint16_t )( spiDmaCfg_[port].txDmaCh << DMA_CITER_ELINKYES_LINKCH_SHIFT ) |
         cnt;
      DMA_BITER_ELINKYES_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) =
         DMA_BITER_ELINKYES_ELINK_MASK |
         ( uint16_t )( spiDmaCfg_[port].txDmaCh << DMA_BITER_ELINKYES_LINKCH_SHIFT ) |
         cnt;

      /* CSR Reg - Configure the DMA SPI RX to go through one Major loop and then interrupt when complete. */
      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = DMA_CSR_INTMAJOR_MASK | DMA_CSR_DREQ_MASK;
      DMA_CSR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = DMA_CSR_DREQ_MASK;

      /* Since the RX & TX interrupts from the SPI are OR'd together, choose the correct one to enable based on whether
         transmitting data from a buffer or receiving data to a buffer!
      */
      if ( NULL != pTxData ) /* Transmitting   */
      {
         DMA_SADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = ( uint32_t )pTxData; /* Set source address in the DMA. */
         DMA_SOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 1;          /* Increment source addr with each txfr */

         DMA_DADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = ( uint32_t )&rxData[0]; /* Set dest address in the DMA. */
         DMA_DOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 0;             /* Increment dest */
      }
      else  /* Receiving   */
      {
         DMA_SADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = ( uint32_t )&txByte_[port]; /* Set src addr in DMA. */
         DMA_SOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].txDmaCh ) = 0;             /* Transmit same byte repeatedly */

         DMA_DADDR_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = ( uint32_t )pRxData;
         DMA_DOFF_REG( DMA_BASE_PTR, spiDmaCfg_[port].rxDmaCh ) = 1;             /* Increment dest by 1 per transfer */
      }

      DMA_SERQ = spiDmaCfg_[port].rxDmaCh;      /* Enable Receive  Channel */
      DMA_SERQ = spiDmaCfg_[port].txDmaCh;      /* Enable Transmit Channel */

      if ( ePWR_LOW == pwrMode_ )
      {
         uint16_t timeOut = 0x100;
         spiIsrFired_[port] = ( bool )false;
         DMA_SERQ = spiDmaCfg_[port].txDmaCh;                                       /* Enable ch - Starts transfer */
         /* Put the processor to sleep, the DMA will wake up when the DMA ISR occurs.  This sleep mode needs to be set
            so that the peripheral devices are running.  Also, note, ALL OTHER INTERRUPTS must be disabled!!! */
         while( !spiIsrFired_[port] && ( 0 != --timeOut ) )
         {
            SLEEP_CPU_IDLE(); /*lint !e522 function has no side effects; OK   */
         }
      }
      else
      {
         // Polling is faster than waiting for semaphore but prevents other tasks from running
         if ( rtosEn_ )
         {
            if ( false == OS_SEM_Pend( &spiSem_[port], SPI_WR_TIME_OUT_mS ) ) /* Pend on DMA ISR completion */
            {
               retVal = eFAILURE;                                             /* Sem time-out, return w/error */
            }
         }
         else
         {
            uint32_t start   = DWT_CYCCNT;
            uint32_t timeout = SPI_WR_TIME_OUT_mS*(getCoreClock()/1000U);

            while( !( DMA_INT & ( ( uint32_t )1 << spiDmaCfg_[port].rxDmaCh ) ) && !spiIsrFired_[port] )
            {
               // Get out if we have been stuck here for a while
               // There is no evidence that this ever happened but better be safe.
               if ( (DWT_CYCCNT - start) > timeout )
               {
                  retVal = eFAILURE;
                  break;
               }
            }
         }
      }
   }  /*lint +esym(550,rxData)  Temp loc to receive data when received data is ignored. */
   return ( retVal );
}
#endif

/* SPI DMA ISRs */

/***********************************************************************************************************************

   Function Name: Spi0DmaIsr

   Purpose: After DMA completes a transfer, this ISR is called.

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if USE_MQX_SPI_DRIVER == 0
void Spi0DmaIsr( void )
{
   SpiDmaIsr( 0 ); /* Call the interrupt handler, pass in SPI 0 */
}
#endif

/***********************************************************************************************************************

   Function Name: Spi1DmaIsr

   Purpose: After DMA completes a transfer, this ISR is called.

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if USE_MQX_SPI_DRIVER == 0
void Spi1DmaIsr( void )
{
   SpiDmaIsr( 1 ); /* Call the interrupt handler, pass in SPI 1 */
}
#endif

/***********************************************************************************************************************

   Function Name: Spi2DmaIsr

   Purpose: After DMA completes a transfer, this ISR is called.

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if USE_MQX_SPI_DRIVER == 0
#if defined ( SPI2_BASE_PTR ) && (HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A)
void Spi2DmaIsr( void )
{
   SpiDmaIsr( 2 ); /* Call the interrupt handler, pass in SPI 2 */
}

#endif
#endif

/***********************************************************************************************************************

   Function Name: SpiDmaIsr

   Purpose: After DMA completes a transfer, this ISR is called.

   Arguments:  uint8_t port - SPI port that fired the DMA interrupt.

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if USE_MQX_SPI_DRIVER == 0
static void SpiDmaIsr( uint8_t port )
{
   DMA_CINT = spiDmaCfg_[port].rxDmaCh;
   DMA_CINT = spiDmaCfg_[port].txDmaCh;
   spiIsrFired_[port] = ( bool )true;
#ifndef __BOOTLOADER
   if ( rtosEn_ )
   {
      OS_SEM_Post( &spiSem_[port] );
   }
#endif   /* BOOTLOADER  */
}
#endif
#endif

/***********************************************************************************************************************

   Function Name: SPI_RtosEnable

   Purpose: Enables or Disables the use of the RTOS semaphores.

   Arguments:  bool rtosEnable - true = Use RTOS semaphores, false = Don't use RTOS semaphores.

   Returns: None

   Side Effects: when rtos is disabled, the driver will not "give up" the processor while waiting for the DMA.

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void SPI_RtosEnable( bool rtosEnable )
{
   rtosEn_ = rtosEnable;
}
#endif   /* BOOTLOADER  */


#ifdef TM_SPI_DRIVER_UNIT_TEST
/***********************************************************************************************************************

   Function Name: SPI_UnitTest

   Purpose: Sends cnt number of bytes out the SPI port.  The data received is not used and discarded.  The receive
   buffer is not read.  The received bytes are counted to ensure the transmission is complete before returning.  This
   allows the driver accessing the SPI port to raise its chip select after all data is sent and not before.

   Arguments:  None

   Returns: None

   Side Effects: Will exercise the SPI port

   Reentrant Code: Yes

 **********************************************************************************************************************/
void SPI_UnitTest( void )
{
   uint8_t          buffer[4096];                 /* Buffer used for spi RX and TX operations. */
   uint8_t          i;                            /* Used for counters. */
   spiCfg_t       spiCfg;                       /* Used for configuration the SPI port. */
   uint8_t          port = SPI_UNIT_TEST_PORT; /* Spi port for unit testing. */
   returnStatus_t spiRes;                       /* Result of the SPI driver API. */
   uint8_t          prntBuf[40];                  /* Used for printing the results. */

   memset( &buffer[0], 0x55, sizeof( buffer ) );
   PRNT_crlf();

   /* Test #1 - Valid Config, Invlaid Port */
   spiCfg.clkBaudRate_Khz = 4000;
   spiCfg.mode = 0;
   spiCfg.txByte = 0;
   spiRes = SPI_OpenPort( 0, &spiCfg, SPI_MASTER );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #2 - Valid Config, Invlaid Port */
   spiRes = SPI_OpenPort( SPI_UNIT_TEST_PORT_INVALID, &spiCfg, SPI_MASTER );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #3 - Valid Config, Invlaid Port */
   spiRes = SPI_OpenPort( 255, &spiCfg, SPI_MASTER );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #4 - Valid Config, Invlaid bit rate */
   spiCfg.clkBaudRate_Khz = 1;   /* 1 KHz */
   spiRes = SPI_OpenPort( port, &spiCfg, SPI_MASTER );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #5 - Valid Config */
   spiCfg.clkBaudRate_Khz = 100;   /* 100 KHz */
   spiRes = SPI_OpenPort( port, &spiCfg, SPI_MASTER );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #6 - Valid Port #  */
   spiRes = SPI_ClosePort( port );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #7 - Invalid Port #  */
   spiRes = SPI_ClosePort( 0 );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #8 - Invalid Port #  */
   spiRes = SPI_ClosePort( SPI_UNIT_TEST_PORT_INVALID );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #9 - Invalid Port #  */
   spiRes = SPI_ClosePort( 255 );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   spiCfg.clkBaudRate_Khz = 100;
   spiCfg.mode = 0;
   spiCfg.txByte = 0;
   spiRes = SPI_OpenPort( SPI_UNIT_TEST_PORT, &spiCfg, SPI_MASTER );


   /* Test #10 - Write Data - Valid Port  */
   spiRes = SPI_WritePort( SPI_UNIT_TEST_PORT, &buffer[0], i++ );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );

   /* Test #11 - Read Data - Valid Port  */
   spiRes = SPI_ReadPort( SPI_UNIT_TEST_PORT, &buffer[0], i++ );
   sprintf( &prntBuf[0], "%d", spiRes );
   PRNT_stringCrLf( &prntBuf[0] );
   PRNT_crlf();

   uint8_t cnt = 2; /* Go through the loop twice, once at 100KHz and once at 4MHz. */
   do
   {
      for ( i = 0; i < 4; i++ )
      {
         spiRes = SPI_WritePort( port, &buffer[0], i );
         sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
         PRNT_stringWaitCr( &prntBuf[0] );
         PRNT_crlf();
      }
      spiRes = SPI_WritePort( port, &buffer[0], 1000 );
      sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
      PRNT_stringWaitCr( &prntBuf[0] );
      PRNT_crlf();

      spiRes = SPI_WritePort( port, &buffer[0], 4096 );
      sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
      PRNT_stringWaitCr( &prntBuf[0] );
      PRNT_crlf();

      for ( i = 0; i < 4; i++ )
      {
         spiRes = SPI_ReadPort( port, &buffer[0], i );
         sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
         PRNT_stringWaitCr( &prntBuf[0] );
         PRNT_crlf();
      }
      spiRes = SPI_ReadPort( port, &buffer[0], 1000 );
      sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
      PRNT_stringWaitCr( &prntBuf[0] );
      PRNT_crlf();

      spiRes = SPI_ReadPort( port, &buffer[0], 4096 );
      sprintf( &prntBuf[0], "%d, Press Any Key to Continue: ", spiRes );
      PRNT_stringWaitCr( &prntBuf[0] );
      PRNT_crlf();

      spiCfg.clkBaudRate_Khz = 4000;
      spiCfg.mode = 0;
      spiCfg.txByte = 0;
      spiRes = SPI_OpenPort( 2, &spiCfg, SPI_MASTER );
   }while(--cnt);

   sprintf( &prntBuf[0], "End Unit Testing" );
   PRNT_stringCrLf( &prntBuf[0] );

   return( eSUCCESS );
}
#endif
