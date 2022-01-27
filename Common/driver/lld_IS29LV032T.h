/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: lld_IS29LV032T.h
 *
 * Contents: Low level flash driver.    We are using the ISSI 29LV032T device.
 *
 *
 *
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 *
 *
 **********************************************************************************************************************/
#ifndef __LLD_IS29LV032T_H
#define __LLD_IS29LV032T_H

typedef uint32_t WORDCOUNT;   /* used for multi-byte operations */

/* polling return status */
typedef enum {
   DEV_STATUS_UNKNOWN = 0,
   DEV_NOT_BUSY,
   DEV_BUSY,
   DEV_EXCEEDED_TIME_LIMITS,
   DEV_SUSPEND,
   DEV_VERIFY_ERROR,
   DEV_ERASE_ERROR,
   DEV_PROGRAM_ERROR,
   DEV_SECTOR_LOCK,
   DEV_PROGRAM_SUSPEND,     /* Device is in program suspend mode */
   DEV_PROGRAM_SUSPEND_ERROR,   /* Device program suspend error */
   DEV_ERASE_SUSPEND,       /* Device is in erase suspend mode */
   DEV_ERASE_SUSPEND_ERROR,   /* Device erase suspend error */
} DEVSTATUS;


typedef uint16_t FLASHDATA;


/* public function prototypes */

/* Operation Functions */
extern FLASHDATA lld_ReadOp ( uint32_t offset );   /* address offset from base address */
extern void lld_ReadBufferOp ( uint32_t offset, FLASHDATA *read_buf, FLASHDATA cnt ); /* pass it a word buffer and number of words to read */
extern DEVSTATUS lld_WriteBufferProgramOp ( uint32_t offset, uint32_t word_cnt, FLASHDATA * data_buf );      /* number of words to program and a buffer of words */
extern DEVSTATUS lld_ProgramOp ( uint32_t offset, FLASHDATA write_data ); /* Pass it the offset and data word to write */
extern DEVSTATUS lld_SectorEraseOp ( uint32_t offset );         /* address offset from base address */
extern DEVSTATUS lld_ChipEraseOp ( void );
extern DEVSTATUS lld_EraseSuspendOp ( FLASHDATA *base_addr, uint32_t offset );
extern DEVSTATUS lld_ProgramSuspendOp ( FLASHDATA *base_addr, uint32_t offset );
extern void lld_InitCmd ( FLASHDATA *init_base_addr );    /* device base address in system */
extern void lld_ResetCmd ( void );
extern uint16_t  lld_GetDeviceId ( void);    /* Returns manufacturer in high byte and device id in low byte */

#endif


