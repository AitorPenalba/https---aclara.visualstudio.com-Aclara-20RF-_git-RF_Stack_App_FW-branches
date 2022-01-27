/*!
 * File:
 *  si446x_api_lib.c
 *
 * Description:
 *  This file contains the Si446x API library.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#include "project.h"
#include <stdarg.h>
#include <mqx.h>
#include "compiler_types.h"
#include "radio_hal.h"
#include "radio_comm.h"
#include "si446x_api_lib.h"
#include "si446x_cmd.h"
#include "si446x_patch.h"
#include "file_io.h"
#include "radio.h" //RADIO_Set_RxStart()

#define RADIO_PATCH_SIZE      576 // Radio Patch size stored in code
#define PATCH_LINE_SIZE         9 // length for each line of code in the header file
#define RADIO_PATCH_MAX_SIZE 1024 // Maximum patch size

#define RADIO_PATCH_FILE_UPDATE_RATE ((uint32_t)0xFFFFFFFF)/* File updates every now and then */

//SEGMENT_VARIABLE( Si446xCmd, union si446x_cmd_reply_union, SEG_XDATA );
//union si446x_cmd_reply_union Si446xCmd;
//SEGMENT_VARIABLE( Pro2Cmd[16], U8, SEG_XDATA );
//uint8_t Pro2Cmd[16];

#ifdef SI446X_PATCH_CMDS
//SEGMENT_VARIABLE( Si446xPatchCommands[][8] = { SI446X_PATCH_CMDS }, U8, SEG_CODE);
//static const uint8_t Si446xPatchCommands[][8] = { SI446X_PATCH_CMDS };
#endif

static FileHandle_t RADIOpatchFileHndl_ = {0};    /*!< Contains the file handle information. */

/*!
 * This functions is used to reset the si446x radio by applying shutdown and
 * releasing it.  After this function @ref si446x_boot should be called.  You
 * can check if POR has completed by waiting 4 ms or by polling GPIO 0, 2, or 3.
 * When these GPIOs are high, it is safe to call @ref si446x_boot.
 */
void si446x_reset(void)
{
    uint8_t i; // Loop counter

    /* Put radios in shutdown, wait then release */
    radio_hal_AssertShutdown((uint8_t)RADIO_0); // First radio on Frodo and Samwise
#if ( DCU == 1 )
    radio_hal_AssertShutdown((uint8_t)RADIO_1); // All RX radios on Frodo
#endif

    // MKD 2019-02-25 1:05PM
    // We observed some infinite reboot of the TB at GVEC after download is applied.
    // We don't know the root cause of the problem but Scot Gingerich looked into it and found that increasing the following sleep to 10 ms was finxing the problem.
    // Might as well make it longer
    OS_TASK_Sleep( FIFTY_MSEC );
    radio_hal_DeassertShutdown((uint8_t)RADIO_0);
#if ( DCU == 1 )
    radio_hal_DeassertShutdown((uint8_t)RADIO_1);
#endif
    for (i=0; i<(uint8_t)MAX_RADIO; i++) {
       radio_comm_ClearCTS(i);
    }
}

/*!
 * This function is used to initialize after power-up the radio chip.
 * Before this function @si446x_reset should be called.
 */
void si446x_power_up(uint8_t radioNum, U8 BOOT_OPTIONS, U8 XTAL_OPTIONS, U32 XO_FREQ)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_POWER_UP;
    Pro2Cmd[1] = BOOT_OPTIONS;
    Pro2Cmd[2] = XTAL_OPTIONS;
    Pro2Cmd[3] = (U8)(XO_FREQ >> 24);
    Pro2Cmd[4] = (U8)(XO_FREQ >> 16);
    Pro2Cmd[5] = (U8)(XO_FREQ >> 8);
    Pro2Cmd[6] = (U8)(XO_FREQ);

    (void)radio_comm_SendCmd(radioNum, SI446X_CMD_ARG_COUNT_POWER_UP, Pro2Cmd );
}

typedef struct
{
   uint8_t radio_patch[RADIO_PATCH_MAX_SIZE];
}radio_patch_t;

/*!
 * This function is used to load all properties and commands with a list of NULL terminated commands.
 * Before this function @si446x_reset should be called.
 */
U8 si446x_configuration_init(uint8_t radioNum, const U8* pSetPropCmd, U16 partInfo)
{
   FileStatus_t fileStatus;
//  SEGMENT_VARIABLE(col, U8, SEG_DATA);
   uint8_t col;
//  SEGMENT_VARIABLE(numOfBytes, U8, SEG_DATA);
   uint8_t numOfBytes;
   union si446x_cmd_reply_union Si446xCmd;
   uint8_t Pro2Cmd[16];

   uint8_t  patchLine[PATCH_LINE_SIZE];
   uint32_t patchLineIndex=0;
//Suppress the compiler warning message for the next statement
#pragma diag_suppress=Pe177
   /* The radio patch is referenced as multiple 16 bytes sections.  The radio patch is allocated as a 1024 byte file.
      No typedef is ever instantiated for the file. A pointer is instantiated here (but never used) so the typedef info
      is included in the DWARF debug info of the output file.  This allows the NV file extraction post-processing
      application to capture the offset info for the file.
   */
   const radio_patch_t *const ppatch=0;

#pragma diag_default=Pe177

   if (partInfo == 0x4460) {
      // Download radio patch from flash
      if ( eSUCCESS == FIO_fopen(&RADIOpatchFileHndl_,             /* File Handle so that RADIO access the file. */
                                 ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                                 (uint16_t)eFN_RADIO_PATCH,        /* File ID (filename) */
                                 (lCnt)RADIO_PATCH_MAX_SIZE,       /* Size of the data in the file. */
                                 FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                                 RADIO_PATCH_FILE_UPDATE_RATE,     /* The update rate of the data in the file. */
                                 &fileStatus) )                    /* Contains the file status */
      {
         // Load first patch line
         (void)FIO_fread( &RADIOpatchFileHndl_, Pro2Cmd, 0, sizeof(patchLine));

         // Patch radio if patch present in flash
         if (Pro2Cmd[0] != 0x00) {
            do {
               // Patch radio
               if (radio_comm_SendCmdGetResp(radioNum, PATCH_LINE_SIZE-1, &Pro2Cmd[1], 0, 0) != 0xFF) {
                  /* Timeout occured */
                  return SI446X_CTS_TIMEOUT;
               }

               // Get next patch line
               patchLineIndex += PATCH_LINE_SIZE;
               (void)FIO_fread( &RADIOpatchFileHndl_, Pro2Cmd, patchLineIndex, sizeof(patchLine));
            } while (Pro2Cmd[0] != 0x00);
            // Adjust pointer to skip patch that is in code space
            pSetPropCmd += RADIO_PATCH_SIZE;
         }
      }
   } else {
      // This is a Si4467/Si4468
      // Adjust pointer to skip patch that is in code space since it is not needed
      pSetPropCmd += RADIO_PATCH_SIZE;

      // Load power up sequence from template
      numOfBytes = *pSetPropCmd++;

      for (col = 0u; col < numOfBytes; col++)
      {
        Pro2Cmd[col] = *pSetPropCmd++;
      }

      // Mask patch option and power up
      Pro2Cmd[1] &= 0x7F; /*lint !e771 */
      (void)radio_comm_SendCmd(radioNum, SI446X_CMD_ARG_COUNT_POWER_UP, Pro2Cmd );
   }

   /* While cycle as far as the pointer points to a command */
   while (*pSetPropCmd != 0x00)
   {
     /* Commands structure in the array:
      * --------------------------------
      * LEN | <LEN length of data>
      */

     numOfBytes = *pSetPropCmd++;

     if (numOfBytes > 16u)
     {
       /* Number of command bytes exceeds maximal allowable length */
       return SI446X_COMMAND_ERROR;
     }

     for (col = 0u; col < numOfBytes; col++)
     {
       Pro2Cmd[col] = *pSetPropCmd;
       pSetPropCmd++;
     }

     if (radio_comm_SendCmdGetResp(radioNum, numOfBytes, Pro2Cmd, 0, 0) != 0xFF)
     {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
     }

     if (radio_hal_NirqLevel() == 0)
     {
       /* Get and clear all interrupts.  An error has occured... */
       (void)si446x_get_int_status(radioNum, 0, 0, 0, &Si446xCmd);
       if (Si446xCmd.GET_INT_STATUS.CHIP_PEND & SI446X_CMD_GET_CHIP_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_MASK)
       {
         return SI446X_COMMAND_ERROR;
       }
     }
   }

   return SI446X_SUCCESS;  /*lint !e438 Last value assigned to variable 'ppatch' not used */
}  /*lint !e550 !e529 argList not accessed   */


/*!
 * This function is used to apply a firmware patch to the si446x radio.  This
 * patch is stored in code using the si446x_patch.h file.
 *
 * @return  SI446X_CTS_TIMEOUT If a CTS error occurs.
 *          SI446X_PATCH_FAIL If the patch fails.
 *          SI446X_SUCCESS If the patch is successful.
 *          SI446X_NO_PATCH If there is no patch in the Flash to load.
 */

#if 0
U8 si446x_apply_patch(void)
{
#ifdef SI446X_PATCH_CMDS
//    SEGMENT_VARIABLE(line, U16, SEG_DATA);
uint16_t line;
//    SEGMENT_VARIABLE(row,  U8, SEG_DATA);
uint8_t row;

    /* Check if patch is needed. */
//    si446x_part_info();

//    if ((Si446xCmd->PART_INFO.ROMID == SI446X_PATCH_ROMID) && ( (U8)( ((U16)Si446xCmd->PART_INFO.ID >> 8) & 0x00FF) < SI446X_PATCH_ID))
    {
      for (line = 0; line < (sizeof(Si446xPatchCommands) / 8u); line++)
      {
        for (row=0; row<8; row++)
        {
          Pro2Cmd[row] = Si446xPatchCommands[line][row];
        }
#if 0
        if (radio_comm_SendCmdGetResp(8, Pro2Cmd, 0, 0) != 0xFF)
        {
          // Timeout occured
          return SI446X_CTS_TIMEOUT;
        }
#endif
        if (radio_hal_NirqLevel() == 0)
        {
          /* Get and clear all interrupts.  An error has occured... */
//          si446x_get_int_status(0, 0, 0);
          return SI446X_PATCH_FAIL;
        }
      }
   }

    return SI446X_SUCCESS;
#else
    return SI446X_NO_PATCH;
#endif
}
#endif
/*! This function sends the PART_INFO command to the radio and receives the answer
 *  into @Si446xCmd union.
 */
uint8_t si446x_part_info(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_PART_INFO;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_PART_INFO,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_PART_INFO,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->PART_INFO.CHIPREV         = Pro2Cmd[0];
       Si446xCmd->PART_INFO.PART            = ((U16)Pro2Cmd[1] << 8) & 0xFF00;
       Si446xCmd->PART_INFO.PART           |= (U16)Pro2Cmd[2] & 0x00FF;
       Si446xCmd->PART_INFO.PBUILD          = Pro2Cmd[3];
       Si446xCmd->PART_INFO.ID              = ((U16)Pro2Cmd[4] << 8) & 0xFF00;
       Si446xCmd->PART_INFO.ID             |= (U16)Pro2Cmd[5] & 0x00FF;
       Si446xCmd->PART_INFO.CUSTOMER        = Pro2Cmd[6];
       Si446xCmd->PART_INFO.ROMID           = Pro2Cmd[7];

       return SI446X_SUCCESS;
    }
}

/*! Sends START_TX command to the radio.
 *
 * @param CHANNEL   Channel number.
 * @param CONDITION Start TX condition.
 * @param TX_LEN    Payload length (exclude the PH generated CRC).
 */
uint8_t si446x_start_tx(uint8_t radioNum, U8 CHANNEL, U8 CONDITION, U16 TX_LEN, uint32_t txtime)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_START_TX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(TX_LEN >> 8);
    Pro2Cmd[4] = (U8)(TX_LEN);
    Pro2Cmd[5] = 0x00;

    // Don't repeat the packet,
    // ie. transmit the packet only once
    Pro2Cmd[6] = 0x00;

    if ( radio_comm_SendCmd_timed( radioNum, SI446X_CMD_ARG_COUNT_START_TX, Pro2Cmd, txtime ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Sends START_RX command to the radio.
 *
 * @param CHANNEL     Channel number.
 * @param CONDITION   Start RX condition.
 * @param RX_LEN      Payload length (exclude the PH generated CRC).
 * @param NEXT_STATE1 Next state when Preamble Timeout occurs.
 * @param NEXT_STATE2 Next state when a valid packet received.
 * @param NEXT_STATE3 Next state when invalid packet received (e.g. CRC error).
 */
uint8_t si446x_start_rx(uint8_t radioNum, U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_START_RX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(RX_LEN >> 8);
    Pro2Cmd[4] = (U8)(RX_LEN);
    Pro2Cmd[5] = NEXT_STATE1;
    Pro2Cmd[6] = NEXT_STATE2;
    Pro2Cmd[7] = NEXT_STATE3;

    RADIO_Set_RxStart(radioNum, DWT_CYCCNT); //Record the timestamp and set the flag

    if ( radio_comm_SendCmd(radioNum, SI446X_CMD_ARG_COUNT_START_RX, Pro2Cmd) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Get the Interrupt status/pending flags form the radio and clear flags if requested.
 *
 * @param PH_CLR_PEND     Packet Handler pending flags clear.
 * @param MODEM_CLR_PEND  Modem Status pending flags clear.
 * @param CHIP_CLR_PEND   Chip State pending flags clear.
 */
uint8_t si446x_get_int_status(uint8_t radioNum, U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_INT_STATUS;
    Pro2Cmd[1] = PH_CLR_PEND;
    Pro2Cmd[2] = MODEM_CLR_PEND;
    Pro2Cmd[3] = CHIP_CLR_PEND;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_INT_STATUS,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_INT_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_INT_STATUS.INT_PEND       = Pro2Cmd[0];
       Si446xCmd->GET_INT_STATUS.INT_STATUS     = Pro2Cmd[1];
       Si446xCmd->GET_INT_STATUS.PH_PEND        = Pro2Cmd[2];
       Si446xCmd->GET_INT_STATUS.PH_STATUS      = Pro2Cmd[3];
       Si446xCmd->GET_INT_STATUS.MODEM_PEND     = Pro2Cmd[4];
       Si446xCmd->GET_INT_STATUS.MODEM_STATUS   = Pro2Cmd[5];
       Si446xCmd->GET_INT_STATUS.CHIP_PEND      = Pro2Cmd[6];
       Si446xCmd->GET_INT_STATUS.CHIP_STATUS    = Pro2Cmd[7];

       return SI446X_SUCCESS;
    }
}

/*!
 * Send GPIO pin config command to the radio and reads the answer into
 * @Si446xCmd union.
 *
 * @param GPIO0       GPIO0 configuration.
 * @param GPIO1       GPIO1 configuration.
 * @param GPIO2       GPIO2 configuration.
 * @param GPIO3       GPIO3 configuration.
 * @param NIRQ        NIRQ configuration.
 * @param SDO         SDO configuration.
 * @param GEN_CONFIG  General pin configuration.
 */
uint8_t si446x_gpio_pin_cfg(uint8_t radioNum, U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GPIO_PIN_CFG;
    Pro2Cmd[1] = GPIO0;
    Pro2Cmd[2] = GPIO1;
    Pro2Cmd[3] = GPIO2;
    Pro2Cmd[4] = GPIO3;
    Pro2Cmd[5] = NIRQ;
    Pro2Cmd[6] = SDO;
    Pro2Cmd[7] = GEN_CONFIG;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GPIO_PIN_CFG,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GPIO_PIN_CFG,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GPIO_PIN_CFG.GPIO[0]        = Pro2Cmd[0];
       Si446xCmd->GPIO_PIN_CFG.GPIO[1]        = Pro2Cmd[1];
       Si446xCmd->GPIO_PIN_CFG.GPIO[2]        = Pro2Cmd[2];
       Si446xCmd->GPIO_PIN_CFG.GPIO[3]        = Pro2Cmd[3];
       Si446xCmd->GPIO_PIN_CFG.NIRQ         = Pro2Cmd[4];
       Si446xCmd->GPIO_PIN_CFG.SDO          = Pro2Cmd[5];
       Si446xCmd->GPIO_PIN_CFG.GEN_CONFIG   = Pro2Cmd[6];

       return SI446X_SUCCESS;
    }
}

/*!
 * Send SET_PROPERTY command to the radio.
 *
 * @param GROUP       Property group.
 * @param NUM_PROPS   Number of property to be set. The properties must be in ascending order
 *                    in their sub-property aspect. Max. 12 properties can be set in one command.
 * @param START_PROP  Start sub-property address.
 */
#ifdef __C51__
#pragma maxargs (13)  /* allow 13 bytes for parameters */
#endif
uint8_t si446x_set_property( uint8_t radioNum, U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... ) /*lint !e579 ... OK   */
{
    va_list argList;
    U8 cmdIndex;

    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_SET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    va_start (argList, START_PROP);
    cmdIndex = 4;
    while(NUM_PROPS--)
    {
        Pro2Cmd[cmdIndex] = va_arg (argList, int);
        cmdIndex++;
    }
    va_end(argList);

    if ( radio_comm_SendCmd( radioNum, cmdIndex, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
        return SI446X_SUCCESS;
    }
}  /*lint !e550 argList not accessed   */

/*!
 * Issue a change state command to the radio.
 *
 * @param NEXT_STATE1 Next state.
 */
uint8_t si446x_change_state(uint8_t radioNum, U8 NEXT_STATE1)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_CHANGE_STATE;
    Pro2Cmd[1] = NEXT_STATE1;

    if ( radio_comm_SendCmd( radioNum, SI446X_CMD_ARG_COUNT_CHANGE_STATE, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       // If state is forced to something other than RX then clear the restart flag because RX state is no longer valid
       if ( NEXT_STATE1 != SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_RX ) {
          RADIO_Clear_RxStartOccured(radioNum);
       }
       return SI446X_SUCCESS;
    }
}


#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
/* Extended driver support functions */
/*!
 * Sends NOP command to the radio. Can be used to maintain SPI communication.
 */
uint8_t si446x_nop(uint8_t radioNum)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_NOP;

    if ( radio_comm_SendCmd( radioNum, SI446X_CMD_ARG_COUNT_NOP, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Send the FIFO_INFO command to the radio. Optionally resets the TX/RX FIFO. Reads the radio response back
 * into @Si446xCmd->
 *
 * @param FIFO  RX/TX FIFO reset flags.
 */
uint8_t si446x_fifo_info(uint8_t radioNum, U8 FIFO, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_FIFO_INFO;
    Pro2Cmd[1] = FIFO;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_FIFO_INFO,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_FIFO_INFO,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->FIFO_INFO.RX_FIFO_COUNT   = Pro2Cmd[0];
       Si446xCmd->FIFO_INFO.TX_FIFO_SPACE   = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

/*!
 * The function can be used to load data into TX FIFO.
 *
 * @param numBytes  Data length to be load.
 * @param pTxData   Pointer to the data (U8*).
 */
void si446x_write_tx_fifo(uint8_t radioNum, U8 numBytes, U8* pTxData)
{
   radio_comm_WriteData( radioNum, SI446X_CMD_ID_WRITE_TX_FIFO, (bool)0, numBytes, pTxData );
}  /*lint !e818 pTxData could be pointer to const   */

/*!
 * Reads the RX FIFO content from the radio.
 *
 * @param numBytes  Data length to be read.
 * @param pRxData   Pointer to the buffer location.
 */
void si446x_read_rx_fifo(uint8_t radioNum, U8 numBytes, U8* pRxData)
{
   radio_comm_ReadData( radioNum, SI446X_CMD_ID_READ_RX_FIFO, (bool)0, numBytes, pRxData );
}

/*!
 * Get property values from the radio. Reads them into Si446xCmd union.
 *
 * @param GROUP       Property group number.
 * @param NUM_PROPS   Number of properties to be read.
 * @param START_PROP  Starting sub-property number.
 */
uint8_t si446x_get_property(uint8_t radioNum, U8 GROUP, U8 NUM_PROPS, U8 START_PROP, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_PROPERTY,
                                    Pro2Cmd,
                                    Pro2Cmd[2],
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_PROPERTY.DATA[0 ]   = Pro2Cmd[0];
       Si446xCmd->GET_PROPERTY.DATA[1 ]   = Pro2Cmd[1];
       Si446xCmd->GET_PROPERTY.DATA[2 ]   = Pro2Cmd[2];
       Si446xCmd->GET_PROPERTY.DATA[3 ]   = Pro2Cmd[3];
       Si446xCmd->GET_PROPERTY.DATA[4 ]   = Pro2Cmd[4];
       Si446xCmd->GET_PROPERTY.DATA[5 ]   = Pro2Cmd[5];
       Si446xCmd->GET_PROPERTY.DATA[6 ]   = Pro2Cmd[6];
       Si446xCmd->GET_PROPERTY.DATA[7 ]   = Pro2Cmd[7];
       Si446xCmd->GET_PROPERTY.DATA[8 ]   = Pro2Cmd[8];
       Si446xCmd->GET_PROPERTY.DATA[9 ]   = Pro2Cmd[9];
       Si446xCmd->GET_PROPERTY.DATA[10]   = Pro2Cmd[10];
       Si446xCmd->GET_PROPERTY.DATA[11]   = Pro2Cmd[11];
       Si446xCmd->GET_PROPERTY.DATA[12]   = Pro2Cmd[12];
       Si446xCmd->GET_PROPERTY.DATA[13]   = Pro2Cmd[13];
       Si446xCmd->GET_PROPERTY.DATA[14]   = Pro2Cmd[14];
       Si446xCmd->GET_PROPERTY.DATA[15]   = Pro2Cmd[15];

       return SI446X_SUCCESS;
    }
}


#ifdef RADIO_DRIVER_FULL_SUPPORT
/* Full driver support functions */

/*!
 * Sends the FUNC_INFO command to the radioNum, then reads the resonse into @Si446xCmd union.
 */
uint8_t si446x_func_info(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_FUNC_INFO;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_FUNC_INFO,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_FUNC_INFO,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->FUNC_INFO.REVEXT          = Pro2Cmd[0];
       Si446xCmd->FUNC_INFO.REVBRANCH       = Pro2Cmd[1];
       Si446xCmd->FUNC_INFO.REVINT          = Pro2Cmd[2];
       Si446xCmd->FUNC_INFO.FUNC            = Pro2Cmd[5];

       return SI446X_SUCCESS;
    }
}

/*!
 * Reads the Fast Response Registers starting with A register into @Si446xCmd union.
 *
 * @param respByteCount Number of Fast Response Registers to be read.
 */
void si446x_frr_a_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    radio_comm_ReadData(radioNum,
                        SI446X_CMD_ID_FRR_A_READ,
                        (bool)0,
                        respByteCount,
                        Pro2Cmd);

    Si446xCmd->FRR_A_READ.FRR_A_VALUE = Pro2Cmd[0];
    Si446xCmd->FRR_A_READ.FRR_B_VALUE = Pro2Cmd[1];
    Si446xCmd->FRR_A_READ.FRR_C_VALUE = Pro2Cmd[2];
    Si446xCmd->FRR_A_READ.FRR_D_VALUE = Pro2Cmd[3];
}

/*!
 * Reads the Fast Response Registers starting with B register into @Si446xCmd union.
 *
 * @param respByteCount Number of Fast Response Registers to be read.
 */
void si446x_frr_b_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    radio_comm_ReadData(radioNum,
                        SI446X_CMD_ID_FRR_B_READ,
                        (bool)0,
                        respByteCount,
                        Pro2Cmd);

    Si446xCmd->FRR_B_READ.FRR_B_VALUE = Pro2Cmd[0];
    Si446xCmd->FRR_B_READ.FRR_C_VALUE = Pro2Cmd[1];
    Si446xCmd->FRR_B_READ.FRR_D_VALUE = Pro2Cmd[2];
    Si446xCmd->FRR_B_READ.FRR_A_VALUE = Pro2Cmd[3];
}

/*!
 * Reads the Fast Response Registers starting with C register into @Si446xCmd union.
 *
 * @param respByteCount Number of Fast Response Registers to be read.
 */
void si446x_frr_c_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    radio_comm_ReadData(radioNum,
                        SI446X_CMD_ID_FRR_C_READ,
                        (bool)0,
                        respByteCount,
                        Pro2Cmd);

    Si446xCmd->FRR_C_READ.FRR_C_VALUE = Pro2Cmd[0];
    Si446xCmd->FRR_C_READ.FRR_D_VALUE = Pro2Cmd[1];
    Si446xCmd->FRR_C_READ.FRR_A_VALUE = Pro2Cmd[2];
    Si446xCmd->FRR_C_READ.FRR_B_VALUE = Pro2Cmd[3];
}

/*!
 * Reads the Fast Response Registers starting with D register into @Si446xCmd union.
 *
 * @param respByteCount Number of Fast Response Registers to be read.
 */
void si446x_frr_d_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    radio_comm_ReadData(radioNum,
                        SI446X_CMD_ID_FRR_D_READ,
                        (bool)0,
                        respByteCount,
                        Pro2Cmd);

    Si446xCmd->FRR_D_READ.FRR_D_VALUE = Pro2Cmd[0];
    Si446xCmd->FRR_D_READ.FRR_A_VALUE = Pro2Cmd[1];
    Si446xCmd->FRR_D_READ.FRR_B_VALUE = Pro2Cmd[2];
    Si446xCmd->FRR_D_READ.FRR_C_VALUE = Pro2Cmd[3];
}

/*!
 * Reads the ADC values from the radio into @Si446xCmd union.
 *
 * @param ADC_EN  ADC enable parameter.
 * @param UDTIME  ADC conversion rate.
 */
uint8_t si446x_get_adc_reading(uint8_t radioNum, U8 ADC_EN, U8 UDTIME, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_ADC_READING;
    Pro2Cmd[1] = ADC_EN;
    Pro2Cmd[2] = (U8)(UDTIME << 4);

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_ADC_READING,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_ADC_READING,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_ADC_READING.GPIO_ADC         = ((U16)Pro2Cmd[0] << 8) & 0xFF00;
       Si446xCmd->GET_ADC_READING.GPIO_ADC        |=  (U16)Pro2Cmd[1] & 0x00FF;
       Si446xCmd->GET_ADC_READING.BATTERY_ADC      = ((U16)Pro2Cmd[2] << 8) & 0xFF00;
       Si446xCmd->GET_ADC_READING.BATTERY_ADC     |=  (U16)Pro2Cmd[3] & 0x00FF;
       Si446xCmd->GET_ADC_READING.TEMP_ADC         = ((U16)Pro2Cmd[4] << 8) & 0xFF00;
       Si446xCmd->GET_ADC_READING.TEMP_ADC        |=  (U16)Pro2Cmd[5] & 0x00FF;

       return SI446X_SUCCESS;
    }
}

/*!
 * Receives information from the radio of the current packet. Optionally can be used to modify
 * the Packet Handler properties during packet reception.
 *
 * @param FIELD_NUMBER_MASK Packet Field number mask value.
 * @param LEN               Length value.
 * @param DIFF_LEN          Difference length.
 */
uint8_t si446x_get_packet_info(uint8_t radioNum, U8 FIELD_NUMBER_MASK, U16 LEN, S16 DIFF_LEN, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_PACKET_INFO;
    Pro2Cmd[1] = FIELD_NUMBER_MASK;
    Pro2Cmd[2] = (U8)(LEN >> 8);
    Pro2Cmd[3] = (U8)(LEN);
    // the different of the byte, althrough it is signed, but to command hander
    // it can treat it as unsigned
    Pro2Cmd[4] = (U8)((U16)DIFF_LEN >> 8);
    Pro2Cmd[5] = (U8)(DIFF_LEN);

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_PACKET_INFO,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_PACKET_INFO,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->PACKET_INFO.LENGTH = ((U16)Pro2Cmd[0] << 8) & 0xFF00;
       Si446xCmd->PACKET_INFO.LENGTH |= (U16)Pro2Cmd[1] & 0x00FF;

       return SI446X_SUCCESS;
    }
}

/*!
 * Gets the Packet Handler status flags. Optionally clears them.
 *
 * @param PH_CLR_PEND Flags to clear.
 */
uint8_t si446x_get_ph_status(uint8_t radioNum, U8 PH_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_PH_STATUS;
    Pro2Cmd[1] = PH_CLR_PEND;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_PH_STATUS,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_PH_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_PH_STATUS.PH_PEND        = Pro2Cmd[0];
       Si446xCmd->GET_PH_STATUS.PH_STATUS      = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

/*!
 * Gets the Modem status flags. Optionally clears them.
 *
 * @param MODEM_CLR_PEND Flags to clear.
 */
uint8_t si446x_get_modem_status(uint8_t radioNum, U8 MODEM_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_MODEM_STATUS;
    Pro2Cmd[1] = MODEM_CLR_PEND;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_MODEM_STATUS,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_MODEM_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_MODEM_STATUS.MODEM_PEND   = Pro2Cmd[0];
       Si446xCmd->GET_MODEM_STATUS.MODEM_STATUS = Pro2Cmd[1];
       Si446xCmd->GET_MODEM_STATUS.CURR_RSSI    = Pro2Cmd[2];
       Si446xCmd->GET_MODEM_STATUS.LATCH_RSSI   = Pro2Cmd[3];
       Si446xCmd->GET_MODEM_STATUS.ANT1_RSSI    = Pro2Cmd[4];
       Si446xCmd->GET_MODEM_STATUS.ANT2_RSSI    = Pro2Cmd[5];
       Si446xCmd->GET_MODEM_STATUS.AFC_FREQ_OFFSET =  ((U16)Pro2Cmd[6] << 8) & 0xFF00;
       Si446xCmd->GET_MODEM_STATUS.AFC_FREQ_OFFSET |= (U16)Pro2Cmd[7] & 0x00FF;

       return SI446X_SUCCESS;
    }
}

/*!
 * Gets the Chip status flags. Optionally clears them.
 *
 * @param CHIP_CLR_PEND Flags to clear.
 */
uint8_t si446x_get_chip_status(uint8_t radioNum, U8 CHIP_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_CHIP_STATUS;
    Pro2Cmd[1] = CHIP_CLR_PEND;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_GET_CHIP_STATUS,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_CHIP_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_CHIP_STATUS.CHIP_PEND         = Pro2Cmd[0];
       Si446xCmd->GET_CHIP_STATUS.CHIP_STATUS       = Pro2Cmd[1];
       Si446xCmd->GET_CHIP_STATUS.CMD_ERR_STATUS    = Pro2Cmd[2];

       return SI446X_SUCCESS;
    }
}

/*!
 * Image rejection calibration.
 *
 * @param SEARCHING_STEP_SIZE
 * @param SEARCHING_RSSI_AVG
 * @param RX_CHAIN_SETTING1
 * @param RX_CHAIN_SETTING2
 */
uint8_t si446x_ircal(uint8_t radioNum, U8 SEARCHING_STEP_SIZE, U8 SEARCHING_RSSI_AVG, U8 RX_CHAIN_SETTING1, U8 RX_CHAIN_SETTING2)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_IRCAL;
    Pro2Cmd[1] = SEARCHING_STEP_SIZE;
    Pro2Cmd[2] = SEARCHING_RSSI_AVG;
    Pro2Cmd[3] = RX_CHAIN_SETTING1;
    Pro2Cmd[4] = RX_CHAIN_SETTING2;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_IRCAL,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_IRCAL,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Image rejection calibration. Forces a specific value for IR calibration, and reads back calibration values from previous calibrations
 *
 * @param IRCAL_AMP
 * @param IRCAL_PH
 */
uint8_t si446x_ircal_manual(uint8_t radioNum, U8 IRCAL_AMP, U8 IRCAL_PH, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_IRCAL_MANUAL;
    Pro2Cmd[1] = IRCAL_AMP;
    Pro2Cmd[2] = IRCAL_PH;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_IRCAL_MANUAL,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_IRCAL_MANUAL,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->IRCAL_MANUAL.IRCAL_AMP_REPLY   = Pro2Cmd[0];
       Si446xCmd->IRCAL_MANUAL.IRCAL_PH_REPLY    = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

/*!
 * Requests the current state of the device and lists pending TX and RX requests
 */
uint8_t si446x_request_device_state(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_REQUEST_DEVICE_STATE;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    SI446X_CMD_ARG_COUNT_REQUEST_DEVICE_STATE,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_REQUEST_DEVICE_STATE,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->REQUEST_DEVICE_STATE.CURR_STATE       = Pro2Cmd[0];
       Si446xCmd->REQUEST_DEVICE_STATE.CURRENT_CHANNEL  = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

/*!
 * While in TX state this will hop to the frequency specified by the parameters
 *
 * @param INTE      New INTE register value.
 * @param FRAC2     New FRAC2 register value.
 * @param FRAC1     New FRAC1 register value.
 * @param FRAC0     New FRAC0 register value.
 * @param VCO_CNT1  New VCO_CNT1 register value.
 * @param VCO_CNT0  New VCO_CNT0 register value.
 * @param PLL_SETTLE_TIME1  New PLL_SETTLE_TIME1 register value.
 * @param PLL_SETTLE_TIME0  New PLL_SETTLE_TIME0 register value.
 */
uint8_t si446x_tx_hop(uint8_t radioNum, U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0, U8 PLL_SETTLE_TIME1, U8 PLL_SETTLE_TIME0)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_TX_HOP;
    Pro2Cmd[1] = INTE;
    Pro2Cmd[2] = FRAC2;
    Pro2Cmd[3] = FRAC1;
    Pro2Cmd[4] = FRAC0;
    Pro2Cmd[5] = VCO_CNT1;
    Pro2Cmd[6] = VCO_CNT0;
    Pro2Cmd[7] = PLL_SETTLE_TIME1;
    Pro2Cmd[8] = PLL_SETTLE_TIME0;

    if ( radio_comm_SendCmd( radioNum, SI446X_CMD_ARG_COUNT_TX_HOP, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * While in RX state this will hop to the frequency specified by the parameters and start searching for a preamble.
 *
 * @param INTE      New INTE register value.
 * @param FRAC2     New FRAC2 register value.
 * @param FRAC1     New FRAC1 register value.
 * @param FRAC0     New FRAC0 register value.
 * @param VCO_CNT1  New VCO_CNT1 register value.
 * @param VCO_CNT0  New VCO_CNT0 register value.
 */
uint8_t si446x_rx_hop(uint8_t radioNum, U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_RX_HOP;
    Pro2Cmd[1] = INTE;
    Pro2Cmd[2] = FRAC2;
    Pro2Cmd[3] = FRAC1;
    Pro2Cmd[4] = FRAC0;
    Pro2Cmd[5] = VCO_CNT1;
    Pro2Cmd[6] = VCO_CNT0;

    if ( radio_comm_SendCmd( radioNum, SI446X_CMD_ARG_COUNT_RX_HOP, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*! Sends START_TX command ID to the radio with no input parameters
 *
 */
uint8_t si446x_start_tx_fast( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_START_TX;

    if ( radio_comm_SendCmd( radioNum, 1, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Sends START_RX command ID to the radio with no input parameters
 *
 */
uint8_t si446x_start_rx_fast( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_START_RX;

    if ( radio_comm_SendCmd( radioNum, 1, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Clear all Interrupt status/pending flags. Does NOT read back interrupt flags
 *
 */
uint8_t si446x_get_int_status_fast_clear( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_INT_STATUS;

    if ( radio_comm_SendCmd( radioNum, 1, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Clear and read all Interrupt status/pending flags
 *
 */
uint8_t si446x_get_int_status_fast_clear_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_INT_STATUS;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    1,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_INT_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_INT_STATUS.INT_PEND       = Pro2Cmd[0];
       Si446xCmd->GET_INT_STATUS.INT_STATUS     = Pro2Cmd[1];
       Si446xCmd->GET_INT_STATUS.PH_PEND        = Pro2Cmd[2];
       Si446xCmd->GET_INT_STATUS.PH_STATUS      = Pro2Cmd[3];
       Si446xCmd->GET_INT_STATUS.MODEM_PEND     = Pro2Cmd[4];
       Si446xCmd->GET_INT_STATUS.MODEM_STATUS   = Pro2Cmd[5];
       Si446xCmd->GET_INT_STATUS.CHIP_PEND      = Pro2Cmd[6];
       Si446xCmd->GET_INT_STATUS.CHIP_STATUS    = Pro2Cmd[7];

       return SI446X_SUCCESS;
    }
}

/*!
 * Reads back current GPIO pin configuration. Does NOT configure GPIO pins
  *
 */
uint8_t si446x_gpio_pin_cfg_fast(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GPIO_PIN_CFG;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    1,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GPIO_PIN_CFG,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GPIO_PIN_CFG.GPIO[0]        = Pro2Cmd[0];
       Si446xCmd->GPIO_PIN_CFG.GPIO[1]        = Pro2Cmd[1];
       Si446xCmd->GPIO_PIN_CFG.GPIO[2]        = Pro2Cmd[2];
       Si446xCmd->GPIO_PIN_CFG.GPIO[3]        = Pro2Cmd[3];
       Si446xCmd->GPIO_PIN_CFG.NIRQ         = Pro2Cmd[4];
       Si446xCmd->GPIO_PIN_CFG.SDO          = Pro2Cmd[5];
       Si446xCmd->GPIO_PIN_CFG.GEN_CONFIG   = Pro2Cmd[6];

       return SI446X_SUCCESS;
    }
}


/*!
 * Clear all Packet Handler status flags. Does NOT read back interrupt flags
 *
 */
uint8_t si446x_get_ph_status_fast_clear( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_PH_STATUS;
    Pro2Cmd[1] = 0;

    if ( radio_comm_SendCmd( radioNum, 2, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Clear and read all Packet Handler status flags.
 *
 */
uint8_t si446x_get_ph_status_fast_clear_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_PH_STATUS;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    1,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_PH_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_PH_STATUS.PH_PEND        = Pro2Cmd[0];
       Si446xCmd->GET_PH_STATUS.PH_STATUS      = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

/*!
 * Clear all Modem status flags. Does NOT read back interrupt flags
 *
 */
uint8_t si446x_get_modem_status_fast_clear( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_MODEM_STATUS;
    Pro2Cmd[1] = 0;

    if ( radio_comm_SendCmd( radioNum, 2, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Clear and read all Modem status flags.
 *
 */
uint8_t si446x_get_modem_status_fast_clear_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_MODEM_STATUS;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                     1,
                                     Pro2Cmd,
                                     SI446X_CMD_REPLY_COUNT_GET_MODEM_STATUS,
                                     Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_MODEM_STATUS.MODEM_PEND   = Pro2Cmd[0];
       Si446xCmd->GET_MODEM_STATUS.MODEM_STATUS = Pro2Cmd[1];
       Si446xCmd->GET_MODEM_STATUS.CURR_RSSI    = Pro2Cmd[2];
       Si446xCmd->GET_MODEM_STATUS.LATCH_RSSI   = Pro2Cmd[3];
       Si446xCmd->GET_MODEM_STATUS.ANT1_RSSI    = Pro2Cmd[4];
       Si446xCmd->GET_MODEM_STATUS.ANT2_RSSI    = Pro2Cmd[5];
       Si446xCmd->GET_MODEM_STATUS.AFC_FREQ_OFFSET = ((U16)Pro2Cmd[6] << 8) & 0xFF00;
       Si446xCmd->GET_MODEM_STATUS.AFC_FREQ_OFFSET |= (U16)Pro2Cmd[7] & 0x00FF;

       return SI446X_SUCCESS;
    }
}

/*!
 * Clear all Chip status flags. Does NOT read back interrupt flags
 *
 */
uint8_t si446x_get_chip_status_fast_clear( uint8_t radioNum )
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_CHIP_STATUS;
    Pro2Cmd[1] = 0;

    if ( radio_comm_SendCmd( radioNum, 2, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Clear and read all Chip status flags.
 *
 */
uint8_t si446x_get_chip_status_fast_clear_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_GET_CHIP_STATUS;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    1,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_GET_CHIP_STATUS,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->GET_CHIP_STATUS.CHIP_PEND         = Pro2Cmd[0];
       Si446xCmd->GET_CHIP_STATUS.CHIP_STATUS       = Pro2Cmd[1];
       Si446xCmd->GET_CHIP_STATUS.CMD_ERR_STATUS    = Pro2Cmd[2];

       return SI446X_SUCCESS;
    }
}

/*!
 * Resets the RX/TX FIFO. Does not read back anything from TX/RX FIFO
 *
 */
uint8_t si446x_fifo_info_fast_reset( uint8_t radioNum, U8 FIFO)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_FIFO_INFO;
    Pro2Cmd[1] = FIFO;

    if ( radio_comm_SendCmd( radioNum, 2, Pro2Cmd ) ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       return SI446X_SUCCESS;
    }
}

/*!
 * Reads RX/TX FIFO count space. Does NOT reset RX/TX FIFO
 *
 */
uint8_t si446x_fifo_info_fast_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd)
{
    uint8_t Pro2Cmd[16];

    Pro2Cmd[0] = SI446X_CMD_ID_FIFO_INFO;

    if ( radio_comm_SendCmdGetResp( radioNum,
                                    1,
                                    Pro2Cmd,
                                    SI446X_CMD_REPLY_COUNT_FIFO_INFO,
                                    Pro2Cmd ) != 0xFF ) {
       /* Timeout occured */
       return SI446X_CTS_TIMEOUT;
    } else {
       Si446xCmd->FIFO_INFO.RX_FIFO_COUNT   = Pro2Cmd[0];
       Si446xCmd->FIFO_INFO.TX_FIFO_SPACE   = Pro2Cmd[1];

       return SI446X_SUCCESS;
    }
}

#endif /* RADIO_DRIVER_FULL_SUPPORT */

#endif /* RADIO_DRIVER_EXTENDED_SUPPORT */
