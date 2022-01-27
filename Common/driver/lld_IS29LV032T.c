// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************

   Filename:   lld_IS29LV032T.c

   Contents: low level flash driver

             The flash driver is a 16 bit device.   All accesses must start on a 16 bit boundary and all operations are in words

             The layers above this code work in bytes and byte addresses.

             for example if a region is supposed to start at Flashbase + 0x1000, this is really at address 0x500 in the flash.

             We do all the conversions in this file.  the low level driver (lld) flash code operates exlusively in words

             The code in this module is not re-entrant.

 ***********************************************************************************************************************
   Copyright (c) 2011-2015 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$


 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
#include <stdint.h>

#include "lld_IS29LV032T.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define DQ2_MASK   (0x04)
#define DQ5_MASK   (0x20)
#define DQ6_MASK   (0x40)


/* LLD Command Definition */
#define NOR_CHIP_ERASE_CMD               (0x10)
#define NOR_ERASE_SETUP_CMD              (0x80)
#define NOR_RESET_CMD                    (0xF0)
#define NOR_SECTOR_ERASE_CMD             (0x30)

/* Command code definition */
#define NOR_AUTOSELECT_CMD               (0x90)
#define NOR_PROGRAM_CMD                  (0xA0)
#define NOR_UNLOCK_DATA1                 (0xAA)
#define NOR_UNLOCK_DATA2                 (0x55)


#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA


/*****************************************************
* Define Flash read/write macro to be used by LLD    *
*****************************************************/
#define FLASH_OFFSET(b,o)       (*(( (volatile FLASHDATA*)(b) ) + (o)))
#define FLASH_WR(b,o,d) (FLASH_OFFSET((b),(o)) = (d))
#define FLASH_RD(b,o)   (FLASH_OFFSET((b),(o)))


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */


typedef enum {
   POLLTYPE_PROGRAM,
   POLLTYPE_ERASE_SECTOR,
   POLLTYPE_ERASE_CHIP
} PollType_e;

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static uint16_t   * base_addr;           /* location of the flash */



// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions">
/* ****************************************************************************************************************** */
/* PRIVATE FUNCTION DEFINITIONS */
static void lld_SectorEraseCmd ( uint32_t offset );
static void lld_ChipEraseCmd ( void );
static void lld_ProgramCmd ( uint32_t offset, FLASHDATA *pgm_data_ptr );
static DEVSTATUS lld_StatusGet ( uint32_t offset );
static DEVSTATUS lld_Poll (
   uint32_t offset,           /* address offset from base address */
   FLASHDATA *exp_data_ptr,    /* expect data */
   FLASHDATA *act_data_ptr,     /* actual data */
   PollType_e polltype
   );


/***********************************************************************************************************************

   Function Name: lld_InitCmd

   Purpose: Init Function

   Arguments: Base Address of flash

   Returns: nothing

   Side Effects: none

   Reentrant Code: yes

 **********************************************************************************************************************/
void lld_InitCmd ( FLASHDATA *init_base_addr )
{
   base_addr = init_base_addr;
}

/***********************************************************************************************************************

   Function Name: lld_ResetCmd

   Purpose: Resets the flash device and puts into read mode.

   Arguments: none

   Returns: nothing

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
void lld_ResetCmd ( void )
{
   /* Write Software RESET command */
   FLASH_WR(base_addr, 0, NOR_RESET_CMD);
}

/***********************************************************************************************************************

   Function Name: lld_SectorEraseCmd

   Purpose: Low level routine to send the sector erase command.
            Erase status polling is not implemented in this function.
   Arguments: none

   Returns: nothing

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
static void lld_SectorEraseCmd ( uint32_t offset )
{

   /* Issue unlock sequence command */
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);

   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD);

   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);

   /* Write Sector Erase Command to Offset */
   FLASH_WR(base_addr, offset, NOR_SECTOR_ERASE_CMD);

}


/***********************************************************************************************************************

   Function Name: lld_ChipEraseCmd

   Purpose: Low level routine to send the chip erase command.
            Erase status polling is not implemented in this function.
   Arguments: none

   Returns: nothing

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
static void lld_ChipEraseCmd ( void )
{
   /* Issue inlock sequence command */
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);

   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD);

   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);

   /* Write Chip Erase Command to Base Address */
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_CHIP_ERASE_CMD);
}


/*lint -efunc( 818, lld_ProgramCmd )  */
/***********************************************************************************************************************

   Function Name: lld_ProgramCmd

   Purpose: Low level routine to program a word into flash.

   Arguments: word offset and data word

   Returns: nothing

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
static void lld_ProgramCmd
(
   uint32_t offset,                  /* address offset from base address */
   FLASHDATA *pgm_data_ptr          /* variable containing data to program */
   )
{
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);

   /* Write Program Command */
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_PROGRAM_CMD);
   /* Write Data */
   FLASH_WR(base_addr, offset, *pgm_data_ptr);
}


/***********************************************************************************************************************

   Function Name: lld_GetDeviceId

   Purpose: Low level routine to retrieve manufacturer and Device ID

   Arguments: none

   Returns: nothing

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
uint16_t lld_GetDeviceId( void )
{
   uint16_t id;

   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
   FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_AUTOSELECT_CMD);

   id =    (uint8_t)(FLASH_RD(base_addr,  0x0100) & 0x000000FF) << 8;
   id |=   (uint8_t)(FLASH_RD(base_addr, 0x0001) & 0x000000FF);

   /* Write Software RESET command */
   FLASH_WR(base_addr, 0, NOR_RESET_CMD);

   return (id);
}

/*lint -efunc( 715, lld_Poll ) : Polltype is not used.  We may want it as we add more flash functions. */
/*lint -efunc( 818, lld_Poll ) : Parameters could be defined as pointer to constant.   It is a part of the common API. */
/***********************************************************************************************************************

   Function Name: lld_Poll

   Purpose: Low level routine to poll flash state.

   Arguments: word offset we are operating on and the type of polling.

   Returns: DEVSTATUS enumeration.

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
static DEVSTATUS lld_Poll
(
   uint32_t offset,                /* address offset from base address */
   FLASHDATA *exp_data_ptr,        /* expect data */
   FLASHDATA *act_data_ptr,        /* actual data */
   PollType_e polltype
   )
{
   DEVSTATUS       dev_status;
   unsigned long   polling_counter = 0xFFFFFFFF;

   do
   {
      polling_counter--;
      dev_status = lld_StatusGet(offset);
   }
   while ((dev_status == DEV_BUSY) && polling_counter);

   /* if device returns status other than "not busy" then we
      have a polling error state. */
   if (dev_status != DEV_NOT_BUSY)
   {
      FLASH_WR(base_addr, 0, NOR_RESET_CMD);
      /* indicate to caller that there was an error */
      return (dev_status);
   }
   else
   {
      /* read the actual data */
      *act_data_ptr = FLASH_RD(base_addr, offset);

      /* Check that polling location verifies correctly */
      if ((*exp_data_ptr & LLD_DB_READ_MASK) == (*act_data_ptr & LLD_DB_READ_MASK))
      {
         dev_status = DEV_NOT_BUSY;
         return (dev_status);
      }
      else
      {
         dev_status = DEV_VERIFY_ERROR;
         return (dev_status);
      }
   }
}



/*lint -efunc( 818, lld_StatusGet ) : Parameters could be defined as pointer to constant.   It is a part of the common API. */
/***********************************************************************************************************************

   Function Name: lld_StatusGet

   Purpose: Low level routine to return flash state.

   Arguments: word offset we are operating on and the type of polling.

   Returns: DEVSTATUS enumeration.

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
static DEVSTATUS lld_StatusGet ( uint32_t offset )
{
   DEVSTATUS rval = DEV_NOT_BUSY;
   FLASHDATA dq2_toggles;
   FLASHDATA dq6_toggles;

   FLASHDATA status_read_1;
   FLASHDATA status_read_2;
   FLASHDATA status_read_3;

   status_read_1 = FLASH_RD(base_addr, offset);
   status_read_2 = FLASH_RD(base_addr, offset);
   status_read_3 = FLASH_RD(base_addr, offset);

   /* Any DQ6 toggles */
   dq6_toggles = ((status_read_1 ^ status_read_2) &        /* Toggles between read1 and read2 */
                  (status_read_2 ^ status_read_3) &        /* Toggles between read2 and read3 */
                  DQ6_MASK);                              /* Check for DQ6 only */

   /* Any DQ2 toggles */
   dq2_toggles = ((status_read_1 ^ status_read_2) &        /* Toggles between read1 and read2 */
                  (status_read_2 ^ status_read_3) &        /* Toggles between read2 and read3 */
                  DQ2_MASK);                              /* Check for DQ6 only */

   if (dq6_toggles)
   {
      if ((DQ5_MASK & status_read_1) == DQ5_MASK)     /* Check if DQ5 is a 1.  */
         rval = DEV_EXCEEDED_TIME_LIMITS;
      else
         rval = DEV_BUSY;
   }
   else if (dq2_toggles)  /* no DQ6 toggles on all devices */
   {
      rval = DEV_SUSPEND;          /* DQ is toggling */
   }
   return (rval);
}


/*lint -efunc( 818, lld_ReadOp ) : Parameters could be defined as pointer to constant.   It is a part of the common API. */
/***********************************************************************************************************************

   Function Name: lld_ReadOp

   Purpose: Read flash memory

   Arguments: Word Offset

   Returns: the 16 bit flash word

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
FLASHDATA lld_ReadOp ( uint32_t offset )
{
   FLASHDATA data;

   data = FLASH_RD(base_addr, offset);

   return (data);
}


/*lint -efunc( 818, lld_ReadBufferOp ) : Parameters could be defined as pointer to constant.   It is a part of the common API. */
/***********************************************************************************************************************

   Function Name: lld_ReadBufferOp

   Purpose: Read a block of memory out of flash

   Arguments: word offset amd the word count.

   Returns: nothing.

   Side Effects: none

   Reentrant Code: no

 **********************************************************************************************************************/
void lld_ReadBufferOp
(
   uint32_t offset,         /* address offset from base address */
   FLASHDATA *read_buf,    /* read data */
   FLASHDATA cnt           /* read count */
   )
{
   for (uint32_t i = 0; i < cnt; i++)
   {
      read_buf[i] = FLASH_RD(base_addr, offset+i);
   }
}


/*lint -efunc( 818, lld_WriteBufferProgramOp ) : Parameters could be defined as pointer to constant.   It is a part of the common API. */
/***********************************************************************************************************************

   Function Name: lld_WriteBufferProgramOp

   Purpose:
         Function programs a write-buffer overlay of addresses to data passed via <data_buf>.

         Function issues all required commands and polls for completion.

    Arguments
         Databuffer is an array of 16 bit words
         Count is a word count.

   Arguments:

   Returns: DEVSTATUS enumeration.

   Side Effects: waits for operation to complete

   Reentrant Code: no

 **********************************************************************************************************************/
DEVSTATUS lld_WriteBufferProgramOp
(
   uint32_t offset,     /* address offset from base address  */
   uint32_t word_count, /* number of words to program        */
   FLASHDATA *data_buf   /* buffer containing data to program */
   )
{
   DEVSTATUS status = DEV_NOT_BUSY;

   /* don't try with a count of zero */
   if (!word_count)
   {
      return (DEV_NOT_BUSY);
   }

   for (uint32_t i = 0; i < word_count; i++)
   {
      status = lld_ProgramOp(offset + i, data_buf[i]);
      if (status != DEV_NOT_BUSY)
      {
         break;
      }

      if (data_buf[i] != FLASH_RD(base_addr, offset + i ))
      {
          status = DEV_VERIFY_ERROR;
          break;
      }
   }
   return (status);
}


/***********************************************************************************************************************

   Function Name: lld_ProgramOp

   Purpose: Writes a word to flash memory.

   Arguments: word offset amd the word to write.


   Returns: DEVSTATUS

   Side Effects: waits for operation to complete

   Reentrant Code: no

 **********************************************************************************************************************/
DEVSTATUS lld_ProgramOp
(
   uint32_t offset,         /* address offset from base address */
   FLASHDATA write_data    /* variable containing data to program */
   )
{
   FLASHDATA read_data = 0;
   DEVSTATUS status;

   lld_ProgramCmd(offset, &write_data);
   status = lld_Poll(offset, &write_data, &read_data, POLLTYPE_PROGRAM);
   return (status);
}


/***********************************************************************************************************************

   Function Name: lld_SectorEraseOp

   Purpose: Erases a sector and waits for erase to complete.

   Arguments: word offset within the sector  you want erased.


   Returns: DEVSTATUS

   Side Effects: waits for operation to complete

   Reentrant Code: no

 **********************************************************************************************************************/
DEVSTATUS lld_SectorEraseOp ( uint32_t offset )
{
   FLASHDATA         expect_data = (FLASHDATA)0xFFFFFFFF;
   FLASHDATA         actual_data = 0;
   DEVSTATUS         status;

   lld_SectorEraseCmd(offset);

   status = lld_Poll(offset, &expect_data, &actual_data, POLLTYPE_ERASE_SECTOR);
   return (status);
}


/***********************************************************************************************************************

   Function Name: lld_ChipEraseOp

   Purpose: Erases the entire flash and waits for erase to complete.

   Arguments: nothing


   Returns: DEVSTATUS

   Side Effects: waits for operation to complete

   Reentrant Code: no

 **********************************************************************************************************************/
DEVSTATUS lld_ChipEraseOp ( void )
{

   DEVSTATUS status;
   FLASHDATA expect_data = (FLASHDATA)0xFFFFFFFF;
   FLASHDATA actual_data = 0;

   lld_ChipEraseCmd();
   status = lld_Poll(0, &expect_data, &actual_data, POLLTYPE_ERASE_CHIP);

   /* if an error during polling, write RESET command to device */
   if (status != DEV_NOT_BUSY)
      /* Write Software RESET command */
      FLASH_WR(base_addr, 0, NOR_RESET_CMD);
   return (status);
}






