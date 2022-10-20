/*!
 * File:
 *  radio_hal.c
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */


               /* ======================================= *
               *               I N C L U D E              *
               *  ======================================= */
/*lint --e{766}   assert.h not used.   */
#include <assert.h>
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#elif (RTOS_SELECTION == FREE_RTOS)
#include "hal_data.h"
#endif
#include "compiler_types.h"
#include "radio_hal.h"
#include "DBG_SerialDebug.h"
#include "spi_mstr.h"

               /* ======================================= *
               *            D E F I N I T I O N S         *
               *  ======================================= */

/*lint -esym(753,CS_BIT_INFO) not used */
#if ( MCU_SELECTED == NXP_K24 )
struct CS_BIT_INFO
{
   GPIO_MemMapPtr port;    /* Base address of GPIO register set   */
   uint8_t        shift;   /* Bit position for given chip select  */
};
#endif

               /* ======================================= *
               *        G L O B A L   V A R I A B L E S    *
               *  ======================================= */

               /* ======================================= *
               *        L O C A L     V A R I A B L E S   *
               *  ======================================= */

#if ( MCU_SELECTED == NXP_K24 )
/* Port and Bit shifts needed for SELA, SELB, SELC, G2A  */
/* Spi port configuration for use with the radio.  The #defines are located in the HALxxx.h. */
static spiCfg_t _spiCfg =
{
   RADIO_0_SLOW_SPEED_KHZ,     /* SPI Clock Rate */
   RADIO_0_TX_BYTE_WHEN_RX,    /* SPI TX Byte when Receiving*/
   RADIO_0_SPI_MODE            /* SPI Mode */
};
#endif

#if (RTOS_SELECTION == FREE_RTOS)
static OS_SEM_Obj    radioSpiSem_;           /* Semaphore used when waiting for busy signal for external flash ISR */
#endif

/*lint -esym(750,NUM_CS_BITS) not used */
#define NUM_CS_BITS ELEMENTS_OF( csinfo )

               /* ======================================= *
               *        L O C A L   F U N C T I O N S     *
               *  ======================================= */
static void     RadioSDN( uint8_t radioNum, uint8_t state );

               /* ======================================= *
               *        P U B L I C   F U N C T I O N S   *
               *  ======================================= */

void radio_hal_AssertShutdown( uint8_t radioNum )
{
   RadioSDN( radioNum, SIG_ASSERT );
}

void radio_hal_DeassertShutdown( uint8_t radioNum )
{
   RadioSDN( radioNum, SIG_RELEASE );
}

/**********************************************************************************************************************/
/*                                                                                                                    */
/*                                               Aclara SPI driver                                                    */
/*                                                                                                                    */
/**********************************************************************************************************************/
uint8_t radio_hal_ClearNsel( uint8_t radioNum )
{
   if ( radioNum > (uint8_t)MAX_RADIO )
   {
      radioNum = (uint8_t)RADIO_0;
   }
   RADIO_0_CS_ACTIVE();
   return RADIO_0_SPI_PORT_NUM;  /*lint !e438 last value assigned to radioNum not used */
}

void radio_hal_SetNsel(uint8_t port)
{
#if ( MCU_SELECTED == NXP_K24 )
   (void)SPI_ClosePort( port );  /*lint !e522 Not really needed... */
#endif
   RADIO_0_CS_INACTIVE();
}

bool radio_hal_NirqLevel(void)
{
   //return RF_NIRQ;
//   return IF_ADF_SWD; // MKD check this
   return false;
}

void radio_hal_SpiInit( bool fastSPI )
{
   RADIO_0_CS_TRIS();      /* Make GPIO pin for TX radio CS an output and make inactive. */
#if ( MCU_SELECTED == NXP_K24 )
   /*lint --e{506,522} constant Boolean value, lacks side effects */
   assert( SPI_initPort( RADIO_0_SPI_PORT_NUM ) == eSUCCESS );

   if (fastSPI) {
      _spiCfg.clkBaudRate_Khz = RADIO_0_FAST_SPEED_KHZ;
   } else {
      _spiCfg.clkBaudRate_Khz = RADIO_0_SLOW_SPEED_KHZ;
   }
   assert( SPI_OpenPort( RADIO_0_SPI_PORT_NUM, &_spiCfg, (bool)SPI_MASTER ) == eSUCCESS );
#elif ( MCU_SELECTED == RA6E1 )
   assert( ( true == OS_SEM_Create( &radioSpiSem_, 0 ) ) );
   R_BSP_PinAccessEnable();
   /* Open the SPI Port */
   (void)R_SPI_Open (&g_spi1_ctrl, &g_spi1_cfg);
#endif
}

void radio_hal_SpiWriteByte(uint8_t port, uint8_t byteToWrite)
{
#if ( MCU_SELECTED == NXP_K24 )
   assert( SPI_WritePort( port, &byteToWrite, sizeof( byteToWrite ) ) == eSUCCESS ); /*lint !e506 !e522 */
#elif ( MCU_SELECTED == RA6E1 )
   assert( R_SPI_Write(&g_spi1_ctrl, &byteToWrite, sizeof( byteToWrite ), SPI_BIT_WIDTH_8_BITS) == FSP_SUCCESS );
   OS_SEM_Pend( &radioSpiSem_, SPI_WR_TIME_OUT_mS );
#endif
}

uint8_t radio_hal_SpiReadByte(uint8_t port)
{
   uint8_t byteToRead;
#if ( MCU_SELECTED == NXP_K24 )
   assert( SPI_ReadPort( port, &byteToRead, sizeof( byteToRead ) ) == eSUCCESS ); /*lint !e506 !e522 */
#elif ( MCU_SELECTED == RA6E1 )
    R_SPI_Read(&g_spi1_ctrl, &byteToRead, sizeof( byteToRead ), SPI_BIT_WIDTH_8_BITS);
    OS_SEM_Pend( &radioSpiSem_, SPI_WR_TIME_OUT_mS );
#endif

   return byteToRead;
}

void radio_hal_SpiWriteData(uint8_t port, uint8_t byteCount, uint8_t const * pData)
{
#if ( MCU_SELECTED == NXP_K24 )
   assert( SPI_WritePort( port, pData, byteCount ) == eSUCCESS ); /*lint !e506 !e522 */
#elif ( MCU_SELECTED == RA6E1 )
   assert( R_SPI_Write(&g_spi1_ctrl, pData, byteCount, SPI_BIT_WIDTH_8_BITS) == FSP_SUCCESS );
   OS_SEM_Pend( &radioSpiSem_, SPI_WR_TIME_OUT_mS );
#endif
}

void radio_hal_SpiReadData(uint8_t port, uint8_t byteCount, uint8_t* pData)
{
#if ( MCU_SELECTED == NXP_K24 )
   assert( SPI_ReadPort( port, pData, byteCount ) == eSUCCESS ); /*lint !e506 !e522 */
#elif ( MCU_SELECTED == RA6E1 )
   assert( R_SPI_Read(&g_spi1_ctrl, pData, byteCount, SPI_BIT_WIDTH_8_BITS) == FSP_SUCCESS );
   OS_SEM_Pend( &radioSpiSem_, SPI_WR_TIME_OUT_mS );
#endif
}

/***********************************************************************************************************************
   Frodo - multiple radioes support

   RadioSDN - manipulate the radio shutdown pin(s)
   Arguments - Radio number - 0 = TX radio
                              1 - 8 = all other radioes ( shared SDN pin on K64 micro )
               State - SIG_ASSERT or SIG_RELEASE
   Returns:

********************************************************************************************************************* */
/*lint --esym(715,radioNum) not referenced */
static void RadioSDN( uint8_t radioNum, uint8_t state )
{
   if ( state == SIG_ASSERT )
   {
      RADIO_0_SDN_ACTIVE();
   }
   else
   {
      RADIO_0_SDN_INACTIVE();
   }
}

/******************************************************************************

 Function Name: radio_hal_RadioImmediateSDN

 Purpose: This function is to shutdown the radio immediately for Last Gasp to save energy

 Arguments: none

 Returns:

******************************************************************************/
void radio_hal_RadioImmediateSDN(void)
{
   RDO_PA_EN_OFF();  /* Turn Off the Radio PA  */
   RDO_SDN_TRIS();   /* Set PCR MUX for GPIO, Make Output, Shutdown Radio */
   // TODO:DG: Use Active Fn
   RDO_OSC_EN_OFF(); /* Turn Off the Radio OSC */
}
/******************************************************************************

 Function Name: spi_callback

 Purpose: Callback function SPI1 peripheral

 Arguments: spi_callback_args_t

 Returns:

******************************************************************************/
void spi_callback(spi_callback_args_t * p_args)
{
   if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
   {
      OS_SEM_Post_fromISR( &radioSpiSem_ );
   }
}